package nsqd

import (
    "net"
    "errors"
    "sync/atomic"
    "bytes"
    "fmt"
)

/**
 *  协议:
 *    SUB: "  V2"(4byte) + "SUB" + " " + "topicName" + " " + "channelName" + \r\n
 *
 */

var separatorBytes = []byte(" ")

type protocolV2 struct {
    ctx *context
}

func (p *protocolV2) IOLoop(conn net.Conn) error {
    clientID := atomic.AddInt64(&p.ctx.nsqd.clientIDSequence, 1)
    client := newClientV2(clientID, conn, p.ctx)

    for {
        line, err := client.Reader.ReadSlice('\n')
        if err != nil {
            break
        }

        line = line[:len(line)-1]

        if len(line) > 0 && line[len(line)-1] == '\r' {
            line = line[:len(line)-1]
        }

        params := bytes.Split(line, separatorBytes)
        _, err = p.Exec(client, params)
        fmt.Println(err)

        fmt.Println("Protocal(V2): ", client, params)
    }
    conn.Close()
    return nil
}

func (p *protocolV2) Exec(client *clientV2, params[][]byte) ([]byte, error) {
    switch {
    case bytes.Equal(params[0], []byte("SUB")):
        return p.SUB(client, params)
    }
    return nil, errors.New("invalid command")
}

func (p *protocolV2) SUB(client *clientV2, params[][]byte) ([]byte, error) {
    if len(params) < 3 {
        return nil, errors.New("SUB insufficient number of params")
    }
    topicName := string(params[1])
    channelName := string(params[2])
    fmt.Println("topicName = ", topicName)
    fmt.Println("channelName = ", channelName)
    return nil, nil
}

