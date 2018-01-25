package nsqd

import (
    "net"
    "errors"
    "sync/atomic"
    "bytes"
    "protocol"
    "fmt"
)

/**
 *  协议:
 *    SUB: "  V2"(4byte) + "SUB" + " " + "topicName" + " " + "channelName" + \r\n
 *
 */

const (
	frameTypeResponse int32 = 0
	frameTypeError    int32 = 1
	frameTypeMessage  int32 = 2
)

var separatorBytes = []byte(" ")
var okBytes = []byte("OK")

type protocolV2 struct {
    ctx *context
}

func (p *protocolV2) IOLoop(conn net.Conn) error {
    clientID := atomic.AddInt64(&p.ctx.nsqd.clientIDSequence, 1)
    client := newClientV2(clientID, conn, p.ctx)

    go p.messagePump(client)

    for {
        line, err := client.Reader.ReadSlice('\n')
        if err != nil {
            break
        }

        line = line[:len(line)-1]

        if len(line) > 0 && line[len(line)-1] == '\r' {
            line = line[:len(line)-1]
        }

        var response []byte
        params := bytes.Split(line, separatorBytes)
        response, err = p.Exec(client, params)

        if err != nil {
            p.Send(client, frameTypeError, []byte(err.Error()))
            break
        }
        if response != nil {
            err = p.Send(client, frameTypeResponse, response)
            if err != nil {
                break
            }
        }
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

    fmt.Println("topicName = ", topicName, channelName)

    var channel *Channel
    for {
        topic := p.ctx.nsqd.GetTopic(topicName)
        channel = topic.GetChannel(channelName)
        break
    }

    client.Channel = channel
    client.SubEventChan <- channel

    return okBytes, nil
}

func (p *protocolV2) Send(client *clientV2, frameType int32, data []byte) error {
	client.writeLock.Lock()

	_, err := protocol.SendFramedResponse(client, frameType, data)
	if err != nil {
		client.writeLock.Unlock()
		return err
	}

	if frameType != frameTypeMessage {
		err = client.Flush()
	}

	client.writeLock.Unlock()

	return err
}

func (p *protocolV2) messagePump(client *clientV2) {

    var subChannel *Channel
    var memoryMsgChan chan *Message
    var backendMsgChan chan []byte
    var err error
    var buf bytes.Buffer

    subEventChan := client.SubEventChan

    for {

        if subChannel != nil {
            memoryMsgChan = subChannel.memoryMsgChan
            backendMsgChan = subChannel.backend.ReadChan()
        }

        select {
        case subChannel = <-subEventChan:
            subEventChan = nil
        case b := <-backendMsgChan:
            msg, err := decodeMessage(b)
			if err != nil {
				fmt.Println("decodeMessage error - ", err)
				continue
			}
            fmt.Println("read backend ", string(msg.Body))
			err = p.SendMessage(client, msg, &buf)
			if err != nil {
				goto exit
			}
        case msg := <-memoryMsgChan:
            fmt.Println("read memory", string(msg.Body))
            err = p.SendMessage(client, msg, &buf)
            if err != nil {
                goto exit
            }
        }
    }
exit:
    fmt.Println("exit")
}

func (p *protocolV2) SendMessage(client *clientV2, msg *Message, buf *bytes.Buffer) error {
	buf.Reset()
	_, err := msg.WriteTo(buf)
	if err != nil {
		return err
	}

	err = p.Send(client, frameTypeMessage, buf.Bytes())
	if err != nil {
		return err
	}

	return nil
}
