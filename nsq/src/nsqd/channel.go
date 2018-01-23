package nsqd

import (
    "sync"
	"sync/atomic"
	"fmt"
    "github.com/nsqio/go-diskqueue"
)

type Channel struct {
    sync.RWMutex

	messageCount uint64

    topicName   string
    name        string
    backend     BackendQueue

    memoryMsgChan chan *Message
}

func NewChannel(topicName string, channelName string) *Channel {
    c := &Channel {
        topicName:  topicName,
        name:       channelName,
        memoryMsgChan: make(chan *Message, MemQeueSize),
    }

    backendName := getBackendName(topicName, channelName)

    dqLogf := func(level diskqueue.LogLevel, f string, args ...interface{}) {
    }

    c.backend = diskqueue.New(
        backendName,
        DataPath,
        MaxBytesPerFile,
        minValidMsgLength,
        MaxMsgSize,
        SyncEvery,
        SyncTimeout,
        dqLogf,
    )

    return c
}


// PutMessage writes a Message to the queue
func (c *Channel) PutMessage(m *Message) error {
	c.RLock()
	defer c.RUnlock()

	err := c.put(m)
	if err != nil {
		return err
	}

	atomic.AddUint64(&c.messageCount, 1)
	return nil
}

func (c *Channel) put(m *Message) error {
	select {
	case c.memoryMsgChan <- m:
	default:
		b := bufferPoolGet()
		err := writeMessageToBackend(b, m, c.backend)
		bufferPoolPut(b)
		if err != nil {
			fmt.Println("writeMessageToBackend err-%s", err)
			return err
		}
	}
	return nil
}
