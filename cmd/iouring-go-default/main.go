package main

import (
	"fmt"
	"os"

	"github.com/iceber/iouring-go"
)

var str = "io with iouring"

func main() {
	var (
		file *os.File
		iour *iouring.IOURing
		err  error
	)

	iour, err = iouring.New(1)
	if err != nil {
		panic(fmt.Sprintf("new IOURing error: %v", err))
	}
	defer func() { _ = iour.Close() }()

	file, err = os.Create("./tmp.txt")
	if err != nil {
		panic(err)
	}

	ch := make(chan iouring.Result, 1)

	prepRequest := iouring.Write(int(file.Fd()), []byte(str))
	if _, err = iour.SubmitRequest(prepRequest, ch); err != nil {
		panic(err)
	}

	result := <-ch
	i, err := result.ReturnInt()
	if err != nil {
		fmt.Println("write error: ", err)
		return
	}

	fmt.Printf("write byte: %d\n", i)
}
