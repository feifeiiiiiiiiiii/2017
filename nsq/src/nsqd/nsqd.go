package nsqd

import (
    "sync"
    "net"
    "os"
    "fmt"
    "util"
    "protocol"
)

type NSQD struct {
    clientIDSequence int64

    sync.RWMutex
    topicMap        map[string]*Topic
    tcpListener     net.Listener

    waitGroup       util.WaitGroupWrapper
}

func New() *NSQD {
    n := &NSQD{
        topicMap: make(map[string]*Topic),
    }

    return n
}

func (n *NSQD) Main() {
    ctx := &context{n}
    tcpListener, err := net.Listen("tcp", "127.0.0.1:3000")
    if err != nil {
        fmt.Println("Main listen err - %s", err)
        os.Exit(1)
    }
    n.Lock()
    n.tcpListener = tcpListener
    n.Unlock()

    tcpServer := &tcpServer{ctx: ctx}

    protocol.TCPServer(n.tcpListener, tcpServer)
    //n.waitGroup.Wrap(func() {
    //})
}

func (n *NSQD) GetTopic(topicName string) *Topic {
    n.RLock()
    t, ok := n.topicMap[topicName]
    n.RUnlock()
    if ok {
        return t
    }
    n.Lock()
    t , ok = n.topicMap[topicName]
    if ok {
        n.Unlock()
        return t
    }

    t = NewTopic(topicName, &context{n})
    n.topicMap[topicName] = t
    n.Unlock()
    return t
}

