//
// Created by sungup on 23/02/21.
//

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <linux/nvme_ioctl.h>
#include <linux/io_uring.h>
#include <liburing.h>

enum nvme_admin_opcode {
    nvme_admin_delete_sq		= 0x00,
    nvme_admin_create_sq		= 0x01,
    nvme_admin_get_log_page		= 0x02,
    nvme_admin_delete_cq		= 0x04,
    nvme_admin_create_cq		= 0x05,
    nvme_admin_identify		= 0x06,
    nvme_admin_abort_cmd		= 0x08,
    nvme_admin_set_features		= 0x09,
    nvme_admin_get_features		= 0x0a,
    nvme_admin_async_event		= 0x0c,
    nvme_admin_ns_mgmt		= 0x0d,
    nvme_admin_activate_fw		= 0x10,
    nvme_admin_download_fw		= 0x11,
    nvme_admin_dev_self_test	= 0x14,
    nvme_admin_ns_attach		= 0x15,
    nvme_admin_keep_alive		= 0x18,
    nvme_admin_directive_send	= 0x19,
    nvme_admin_directive_recv	= 0x1a,
    nvme_admin_virtual_mgmt		= 0x1c,
    nvme_admin_nvme_mi_send		= 0x1d,
    nvme_admin_nvme_mi_recv		= 0x1e,
    nvme_admin_dbbuf		= 0x7C,
    nvme_admin_format_nvm		= 0x80,
    nvme_admin_security_send	= 0x81,
    nvme_admin_security_recv	= 0x82,
    nvme_admin_sanitize_nvm		= 0x84,
    nvme_admin_get_lba_status	= 0x86,
    nvme_admin_vendor_start		= 0xC0,
};

#define nvme_identify_cns_controller 0x01
#define ID_CTRL_SZ 4096

static inline void prep_nvme_identify_ctrl(struct io_uring_sqe *sqe, int fd,
        __u32 nsid, __u32 cntid, __u32 nvmSetId, const char* buf) {
    struct nvme_uring_cmd *cmd;

    // fill IOUring commands
    sqe->fd = fd;
    sqe->rw_flags = 0;
    sqe->opcode = (__u8) IORING_OP_URING_CMD;
    sqe->user_data = 0;
    sqe->cmd_op = NVME_URING_CMD_ADMIN;

    sqe->ioprio = 0;
    sqe->addr = 0;
    sqe->len = 0;
    sqe->user_data = 0;
    sqe->buf_index = 0;
    sqe->personality = 0;
    sqe->file_index = 0;

    // fill NVMe driver commands
    cmd = (struct nvme_uring_cmd*)sqe->cmd;

    memset(cmd, 0, sizeof(*cmd));
    cmd->opcode = nvme_admin_identify;
    cmd->nsid = nsid;
    cmd->cdw10 = (__u32)(cntid) << 16 | (__u32)(nvme_identify_cns_controller);
    cmd->cdw11 = nvmSetId;

    cmd->addr = (__u64)buf;
    cmd->data_len = ID_CTRL_SZ;
}

int main(int argc, char* argv[]) {
    int fd, qd;
    struct nvme_uring_cmd cmd;
    struct io_uring ring;
    struct io_uring_sqe* sqe;
    struct io_uring_cqe* cqe;
    char buf[ID_CTRL_SZ];
    int ret = 0;

    __u32 nsid = 0;
    __u32 cntid = 0;
    __u32 nvmSetId = 0;

    if (argc < 2) {
        printf("Usage: %s <device>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0){
        perror("open");
        return 1;
    }

    qd = io_uring_queue_init(16, &ring, IORING_SETUP_SQE128|IORING_SETUP_CQE32);
    if (qd < 0) {
        perror("io_uring_queue_init");
        ret = 1;
        goto CLOSE_FILE;
    }

    sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        perror("io_uring_get_sqe");
        ret = 1;
        goto QUEUE_EXIT;
    }

    memset(buf, 0, ID_CTRL_SZ);
    prep_nvme_identify_ctrl(sqe, fd, nsid, cntid, nvmSetId, buf);

    io_uring_sqe_set_data(sqe, &cmd);
    ret = io_uring_submit(&ring);
    if (ret < 0) {
        perror("io_uring_submit");
        ret = 1;
        goto QUEUE_EXIT;
    } else {
        printf("return code: %d\n", ret);
    }

    io_uring_wait_cqe(&ring, &cqe);
    printf("Result: %d\n", cqe->res);
    io_uring_cqe_seen(&ring, cqe);

    printf("VID: %x%x\n", buf[1], buf[0]);
    printf("Controller Model Number: %.40s\n", buf+4);

QUEUE_EXIT:
    io_uring_queue_exit(&ring);

CLOSE_FILE:
    close(fd);

    return 0;
}