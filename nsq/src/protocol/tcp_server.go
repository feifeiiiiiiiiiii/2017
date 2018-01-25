package protocol

import (
    "net"
    "fmt"
)

type TCPHandler interface {
    Handle(net.Conn)
}

func TCPServer(listener net.Listener, handler TCPHandler) {
    for {
        clientConn, err := listener.Accept()
        if err != nil {
            fmt.Println("accept client - %s", clientConn.RemoteAddr())
        }
        go handler.Handle(clientConn)
    }
}
