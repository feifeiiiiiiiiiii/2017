package nsqd

import (
    "fmt"
    "net"
    "io"
    "protocol"
)

type tcpServer struct {
    ctx *context
}

func (p *tcpServer) Handle(clientConn net.Conn) {
    fmt.Println("Tcp: new client (%s)", clientConn.RemoteAddr())

    buf := make([]byte, 4)
    _, err := io.ReadFull(clientConn, buf)
    if err != nil {
        fmt.Println("failed to read protocol version - ", err)
        return
    }
    protocolMagic := string(buf)

    var prot protocol.Protocol

    switch protocolMagic {
    case "  V2":
        prot = &protocolV2{ctx: p.ctx}
    default:
        fmt.Println("bad protocol magic ", protocolMagic)
        clientConn.Close()
        return
    }

    err = prot.IOLoop(clientConn)
    if err != nil {
        fmt.Println("IOLoop error - ", err)
        return
    }
}
