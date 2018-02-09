package nsqd

import (
	"fmt"
	"github.com/nsqio/go-diskqueue"
	"sync"
	"sync/atomic"
	"time"
	"util"
)

type Topic struct {
	sync.RWMutex

	messageCount uint64
	waitGroup    util.WaitGroupWrapper

	name      string
	backend   BackendQueue
	idFactory *guidFactory

	memoryMsgChan chan *Message

	channelMap map[string]*Channel
	pauseChan  chan bool

	ctx *context

	// 维护channel状态
	channelUpdateChan chan int
}

const (
	DataPath        = "/Users/orz/Workspace/nsqd-data-path"
	MaxBytesPerFile = 1024
	MaxMsgSize      = 1 << 10
	SyncEvery       = 2500
	SyncTimeout     = time.Second * 2
	NODEID          = 123456
	MemQeueSize     = 100
)

func NewTopic(topicName string, ctx *context) *Topic {
	t := &Topic{
		name:              topicName,
		idFactory:         NewGUIDFactory(NODEID),
		memoryMsgChan:     make(chan *Message, MemQeueSize),
		channelMap:        make(map[string]*Channel),
		ctx:               ctx,
		channelUpdateChan: make(chan int),
		pauseChan:         make(chan bool),
	}

	dqLogf := func(level diskqueue.LogLevel, f string, args ...interface{}) {
	}

	t.backend = diskqueue.New(
		topicName,
		DataPath,
		MaxBytesPerFile,
		minValidMsgLength,
		MaxMsgSize,
		SyncEvery,
		SyncTimeout,
		dqLogf,
	)

	t.waitGroup.Wrap(func() { t.messagePump() })

	return t
}

func (t *Topic) messagePump() {
	var backendChan chan []byte
	var memoryMsgChan chan *Message
	var buf []byte
	var msg *Message
	var err error
	var chans []*Channel

	// copy出来，不要直接对channelMap操作
	t.RLock()
	for _, c := range t.channelMap {
		chans = append(chans, c)
	}
	t.RUnlock()

	if len(chans) > 0 {
		backendChan = t.backend.ReadChan()
		memoryMsgChan = t.memoryMsgChan
	}

	fmt.Println("topic messagePump")

	for {
		select {
		case <-t.channelUpdateChan:
			chans = chans[:0]
			t.RLock()
			for _, c := range t.channelMap {
				chans = append(chans, c)
			}
			t.RUnlock()
			if len(chans) > 0 {
				backendChan = t.backend.ReadChan()
				memoryMsgChan = t.memoryMsgChan
			} else {
				backendChan = nil
				memoryMsgChan = nil
			}
			continue
		case msg = <-memoryMsgChan:
		case buf = <-backendChan:
			msg, err = decodeMessage(buf)
			if err != nil {
				fmt.Println("decodeMessage: error - %s", err)
				continue
			}
		case pause := <-t.pauseChan:
			fmt.Println(pause)
			continue
		}
		// 把消息均等的放入每个channel中
		for i, channel := range chans {
			chanMsg := msg
			if i > 0 {
				chanMsg = NewMessage(msg.ID, msg.Body)
				chanMsg.Timestamp = msg.Timestamp
				chanMsg.deferred = msg.deferred
			}
			err := channel.PutMessage(chanMsg)
			if err != nil {
				fmt.Println("channel putmessage err - %s", err)
			}
		}
	}
}

func (t *Topic) PutMessage(m *Message) error {
	t.RLock()
	defer t.RUnlock()

	err := t.put(m)
	if err != nil {
		return err
	}
	atomic.AddUint64(&t.messageCount, 1)
	return nil
}

func (t *Topic) put(m *Message) error {
	select {
	// 先写入内存队列中, 如果内存队列满了，放入磁盘队里中
	case t.memoryMsgChan <- m:
	default:
		b := bufferPoolGet()
		err := writeMessageToBackend(b, m, t.backend)
		if err != nil {
			fmt.Println("writeMessageToBackend error - %s", err)
			return err
		}
	}
	return nil
}

func (t *Topic) GenerateID() MessageID {
retry:
	id, err := t.idFactory.NewGUID()
	if err != nil {
		time.Sleep(time.Millisecond)
		goto retry
	}
	return id.Hex()
}

func (t *Topic) Close() error {
	return t.backend.Close()
}

// this expects the caller to handle locking
func (t *Topic) getOrCreateChannel(channelName string) (*Channel, bool) {
	channel, ok := t.channelMap[channelName]
	if !ok {
		channel = NewChannel(t.name, channelName)
		t.channelMap[channelName] = channel
		return channel, true
	}
	return channel, false
}

func (t *Topic) GetChannel(channelName string) *Channel {
	t.Lock()
	channel, isNew := t.getOrCreateChannel(channelName)
	t.Unlock()
	if isNew {
		fmt.Println("create new channel %s", channelName)
		select {
		case t.channelUpdateChan <- 1:
		}
	}
	return channel
}
