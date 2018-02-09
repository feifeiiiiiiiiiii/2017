package nsqd

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"net"
	"protocol"
	"sync/atomic"
	"unsafe"
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

	messagePumpStartedChan := make(chan bool)
	go p.messagePump(client, messagePumpStartedChan)
	<-messagePumpStartedChan

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

func (p *protocolV2) Exec(client *clientV2, params [][]byte) ([]byte, error) {
	switch {
	case bytes.Equal(params[0], []byte("SUB")):
		return p.SUB(client, params)
	case bytes.Equal(params[0], []byte("FIN")):
		return p.FIN(client, params)
	case bytes.Equal(params[0], []byte("PUB")):
		return p.PUB(client, params)
	}
	return nil, errors.New("invalid command")
}

func (p *protocolV2) PUB(client *clientV2, params [][]byte) ([]byte, error) {
	var err error

	if len(params) < 2 {
		return nil, errors.New("PUB insufficient number of params")
	}

	topicName := string(params[1])

	fmt.Println("PUB topicName = ", topicName)
	bodyLen, err := readLen(client.Reader, client.lenSlice)
	if bodyLen <= 0 {
		return nil, errors.New("PUB invalid message body size")
	}

	messageBody := make([]byte, bodyLen)
	_, err = io.ReadFull(client.Reader, messageBody)
	if err != nil {
		return nil, errors.New("PUB failed to read message body")
	}

	topic := p.ctx.nsqd.GetTopic(topicName)
	msg := NewMessage(topic.GenerateID(), messageBody)
	err = topic.PutMessage(msg)
	if err != nil {
		return nil, errors.New("PUB failed " + err.Error())
	}

	return okBytes, nil
}

func (p *protocolV2) FIN(client *clientV2, params [][]byte) ([]byte, error) {
	if len(params) < 2 {
		return nil, errors.New("FIN insufficient number of params")
	}

	_, err := getMessageID(params[1])
	if err != nil {
		return nil, errors.New("E_INVALID " + err.Error())
	}
	return nil, nil
}

func (p *protocolV2) SUB(client *clientV2, params [][]byte) ([]byte, error) {
	if len(params) < 3 {
		return nil, errors.New("SUB insufficient number of params")
	}
	topicName := string(params[1])
	channelName := string(params[2])

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

	size, err := protocol.SendFramedResponse(client, frameType, data)
	fmt.Println("send size ", size)
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

func (p *protocolV2) messagePump(client *clientV2, startChaned chan bool) {

	var subChannel *Channel
	var memoryMsgChan chan *Message
	var backendMsgChan chan []byte
	var err error
	var buf bytes.Buffer

	subEventChan := client.SubEventChan

	close(startChaned)

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
			msg.Attempts++
			subChannel.StartInFlightTimeout(msg, client.ID)
			err = p.SendMessage(client, msg, &buf)
			if err != nil {
				goto exit
			}
		case msg := <-memoryMsgChan:
			msg.Attempts++
			subChannel.StartInFlightTimeout(msg, client.ID)
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

// validate and cast the bytes on the wire to a message ID
func getMessageID(p []byte) (*MessageID, error) {
	if len(p) != MsgIDLength {
		return nil, errors.New("Invalid Message ID")
	}
	return (*MessageID)(unsafe.Pointer(&p[0])), nil
}

func readLen(r io.Reader, tmp []byte) (int32, error) {
	_, err := io.ReadFull(r, tmp)
	if err != nil {
		return 0, err
	}
	return int32(binary.BigEndian.Uint32(tmp)), nil
}
