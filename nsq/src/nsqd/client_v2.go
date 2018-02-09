package nsqd

import (
	"bufio"
	"net"
	"sync"
)

const defaultBufferSize = 16 * 1024

type clientV2 struct {
	ID  int64
	ctx *context

	net.Conn

	// reading/writing interfaces
	Reader *bufio.Reader
	Writer *bufio.Writer

	ClientId string
	Hostname string

	Channel   *Channel
	writeLock sync.RWMutex

	SubEventChan chan *Channel

	lenBuf   [4]byte
	lenSlice []byte
}

func newClientV2(id int64, conn net.Conn, ctx *context) *clientV2 {
	var identifier string
	if conn != nil {
		identifier, _, _ = net.SplitHostPort(conn.RemoteAddr().String())
	}
	c := &clientV2{
		ID:   id,
		ctx:  ctx,
		Conn: conn,

		Reader: bufio.NewReaderSize(conn, defaultBufferSize),
		Writer: bufio.NewWriterSize(conn, defaultBufferSize),

		ClientId: identifier,
		Hostname: identifier,

		SubEventChan: make(chan *Channel, 1),
	}
	c.lenSlice = c.lenBuf[:]

	return c
}

func (c *clientV2) Flush() error {
	return c.Writer.Flush()
}
