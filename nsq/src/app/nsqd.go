package main

import (
    "nsqd"
    "fmt"
)

func main() {
    topic := nsqd.NewTopic("hello")
	//msg := nsqd.NewMessage(topic.GenerateID(), []byte("bbbb"))
    //err := topic.PutMessage(msg)
    //topic.PutMessage(msg)
    //topic.PutMessage(msg)
    fmt.Println("hello")
    topic.Close()
}
