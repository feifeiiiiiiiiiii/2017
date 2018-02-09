package nsqd

import (
	"errors"
	"fmt"
	"github.com/nsqio/go-diskqueue"
	"sync"
	"sync/atomic"
	"time"
)

const (
	PQSIZE = 1024
)

type Consumer interface {
}

type Channel struct {
	sync.RWMutex

	messageCount uint64

	topicName string
	name      string
	backend   BackendQueue

	memoryMsgChan chan *Message
	clients       map[int64]Consumer

	inFlightMessages map[MessageID]*Message
	inFlightPQ       inFlightPqueue
	inFlightMutex    sync.Mutex
}

func NewChannel(topicName string, channelName string) *Channel {
	c := &Channel{
		topicName:     topicName,
		name:          channelName,
		memoryMsgChan: make(chan *Message, MemQeueSize),
		clients:       make(map[int64]Consumer),
	}

	c.initPQ()

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

func (c *Channel) initPQ() {
	c.inFlightMutex.Lock()
	defer c.inFlightMutex.Unlock()
	c.inFlightMessages = make(map[MessageID]*Message)
	c.inFlightPQ = newInFlightPqueue(PQSIZE)
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
		fmt.Println("channel: writeMessageToBackend")
		if err != nil {
			fmt.Println("channel: writeMessageToBackend err-%s", err)
			return err
		}
	}
	return nil
}

func (c *Channel) AddClient(clientId int64, client Consumer) {
	c.Lock()
	defer c.Unlock()

	_, ok := c.clients[clientId]
	if ok {
		return
	}
	c.clients[clientId] = client
}

func (c *Channel) RemoveClient(clientId int64) {
	c.Lock()
	defer c.Unlock()

	_, ok := c.clients[clientId]
	if !ok {
		return
	}
	delete(c.clients, clientId)
}

func (c *Channel) StartInFlightTimeout(msg *Message, clientID int64) error {
	now := time.Now()
	msg.clientID = clientID
	msg.deliveryTS = now
	msg.pri = now.UnixNano()
	err := c.pushInFlightMessage(msg)
	if err != nil {
		return err
	}
	c.addToInFlightPQ(msg)
	return nil
}

// pushInFlightMessage atomically adds a message to the in-flight dictionary
func (c *Channel) pushInFlightMessage(msg *Message) error {
	c.inFlightMutex.Lock()
	defer c.inFlightMutex.Unlock()

	_, ok := c.inFlightMessages[msg.ID]
	if ok {
		return errors.New("ID already in flight")
	}
	c.inFlightMessages[msg.ID] = msg
	return nil
}

func (c *Channel) addToInFlightPQ(msg *Message) {
	c.inFlightMutex.Lock()
	defer c.inFlightMutex.Unlock()

	c.inFlightPQ.Push(msg)
}
