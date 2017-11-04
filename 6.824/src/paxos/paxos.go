package paxos

//
// Paxos library, to be included in an application.
// Multiple applications will run, each including
// a Paxos peer.
//
// Manages a sequence of agreed-on values.
// The set of peers is fixed.
// Copes with network failures (partition, msg loss, &c).
// Does not store anything persistently, so cannot handle crash+restart.
//
// The application interface:
//
// px = paxos.Make(peers []string, me string)
// px.Start(seq int, v interface{}) -- start agreement on new instance
// px.Status(seq int) (Fate, v interface{}) -- get info about an instance
// px.Done(seq int) -- ok to forget all instances <= seq
// px.Max() int -- highest instance seq known, or -1
// px.Min() int -- instances before this seq have been forgotten
//

import "net"
import "net/rpc"
import "log"

import "os"
import "syscall"
import "sync"
import "sync/atomic"
import "fmt"
import "math/rand"
import "time"
import "strconv"


// px.Status() return values, indicating
// whether an agreement has been decided,
// or Paxos has not yet reached agreement,
// or it was agreed but forgotten (i.e. < Min()).
type Fate int

const (
	OK = "OK"
	Reject = "REJECT"
)

const (
	Decided   Fate = iota + 1
	Pending        // not yet decided.
	Forgotten      // decided but forgotten.
)

type DecideArgs struct {
	Seq 	int
	PNum 	string
	Value 	interface{}
	Me 		int
	Done 	int
}

type DecideReply struct {}

type AcceptArgs struct {
	Seq 	int
	PNum 	string
	Value 	interface{}
}

type AcceptReply struct {
	Err 	string
}

type PrepareArgs struct {
	Seq 	int
	PNum 	string
}

type PrepareReply struct {
	Err 	string
	A_num 	string
	A_value	interface{}
}

type State struct {
	state 	Fate 
	n_p   	string 	// 提出的
	n_a   	string 	// 建议的
	n_v   	interface{}
}

type Paxos struct {
	mu         sync.Mutex
	l          net.Listener
	dead       int32 // for testing
	unreliable int32 // for testing
	rpcCount   int32 // for testing
	peers      []string
	me         int // index into peers[]


	// Your data here.
	dones 		[]int
	instances 	map[int]*State
}

//
// call() sends an RPC to the rpcname handler on server srv
// with arguments args, waits for the reply, and leaves the
// reply in reply. the reply argument should be a pointer
// to a reply structure.
//
// the return value is true if the server responded, and false
// if call() was not able to contact the server. in particular,
// the replys contents are only valid if call() returned true.
//
// you should assume that call() will time out and return an
// error after a while if it does not get a reply from the server.
//
// please use call() to send all RPCs, in client.go and server.go.
// please do not change this function.
//
func call(srv string, name string, args interface{}, reply interface{}) bool {
	c, err := rpc.Dial("unix", srv)
	if err != nil {
		err1 := err.(*net.OpError)
		if err1.Err != syscall.ENOENT && err1.Err != syscall.ECONNREFUSED {
			fmt.Printf("paxos Dial() failed: %v\n", err1)
		}
		return false
	}
	defer c.Close()

	err = c.Call(name, args, reply)
	if err == nil {
		return true
	}

	fmt.Println(err)
	return false
}


//
// the application wants paxos to start agreement on
// instance seq, with proposed value v.
// Start() returns right away; the application will
// call Status() to find out if/when agreement
// is reached.
//
func (px *Paxos) Start(seq int, v interface{}) {
	// Your code here.
	px.proposer(seq, v)
}

func (px *Paxos) generatePNum() string {
	begin := time.Date(2015, time.May, 6, 22, 0, 0, 0, time.UTC)
	duration := time.Now().Sub(begin)
	return strconv.FormatInt(duration.Nanoseconds(), 10) + "-" + strconv.Itoa(px.me)
}

func (px *Paxos) proposer(seq int, v interface{}) {
	var replyPnum string
	var replyValue interface{}

	for {
		num := 0
		replyPnum = ""
		replyValue = v
		for i := 0; i < len(px.peers); i++ {
			// send prepare
			reply, _ := px.prepare(i, seq, v)
			if reply.Err == OK {
				num += 1
				if reply.A_num > replyPnum  {
					replyPnum = reply.A_num
					replyValue = reply.A_value
				}
			}
		}
		if num >= (len(px.peers)/2 + 1) {
			break
		}
	}
	// go accept

	ok := px.accept(seq, replyPnum, replyValue)
	if ok == true {
		px.decide(seq, replyPnum, replyValue)
	}
}

func (px *Paxos) decide(seq int, pnum string, v interface{}) {
	args := &DecideArgs {
		Seq: seq,
		PNum: pnum,
		Value: v,
		Me: px.me,
		Done: px.dones[px.me],
	}
	for i := 0; i < len(px.peers); i++ {
		var reply DecideReply
		call(px.peers[i], "Paxos.DecideHandler", args, &reply)
	}
}

func (px *Paxos) DecideHandler(args *DecideArgs, repl *DecideReply) error {
	px.mu.Lock()
	defer px.mu.Unlock()

	_, ok := px.instances[args.Seq]
	if !ok {
		state := &State{
			state: Pending,
			n_p: "",
			n_v: "",
			n_a: "",
		}
		px.instances[args.Seq] = state
	}
	px.instances[args.Seq].state = Decided
	px.instances[args.Seq].n_p = args.PNum
	px.instances[args.Seq].n_a = args.PNum
	px.instances[args.Seq].n_v = args.Value
	px.dones[args.Me] = args.Done

	return nil
}

func (px *Paxos) accept(seq int, pnum string, v interface{}) bool {
	args := &AcceptArgs{
		Seq: seq,
		PNum: pnum,
		Value: v,
	}
	num := 0
	for i := 0; i < len(px.peers); i++ {
		var reply AcceptReply
		ok := call(px.peers[i], "Paxos.AcceptHandler", args, &reply)
		if ok == true && reply.Err == OK {
			num += 1
		}
	}

	return num >= (len(px.peers)/2+1)
}

func (px *Paxos) AcceptHandler(args *AcceptArgs, reply *AcceptReply) error {
	px.mu.Lock()
	defer px.mu.Unlock()

	state, ok := px.instances[args.Seq]
	if !ok {
		state = &State{
			state: Pending,
			n_p: "",
			n_v: "",
			n_a: "",
		}
		px.instances[args.Seq] = state
		reply.Err = OK
	} else {
		if args.PNum >= state.n_p {
			reply.Err = OK
		} else {
			reply.Err = Reject
		}
	}
	if reply.Err == OK {
		px.instances[args.Seq].n_a = args.PNum
		px.instances[args.Seq].n_p = args.PNum
		px.instances[args.Seq].n_v = args.Value
	}

	return nil
}

func (px *Paxos) prepare(me int, seq int, v interface{}) (PrepareReply, error) {
	pnum := px.generatePNum()

	args := &PrepareArgs{
		Seq: seq,
		PNum: pnum,
	}

	var reply PrepareReply

	ok := call(px.peers[me], "Paxos.PrepareHandler", args, &reply)
	if ok == false {
		return PrepareReply{Err: Reject}, nil
	}
	return reply, nil
}

func (px *Paxos) PrepareHandler(args *PrepareArgs, reply *PrepareReply) error {
	px.mu.Lock()
	defer px.mu.Unlock()

	state, ok := px.instances[args.Seq]
	if !ok {
		state = &State{
			state: Pending,
			n_p: args.PNum,
			n_a: args.PNum,
			n_v: nil,
		}
		px.instances[args.Seq] = state
		reply.Err = OK
	} else {
		if args.PNum > state.n_p {
			reply.Err = OK
		 } else {
			reply.Err = Reject
		 }
	}

	if reply.Err == OK {
		reply.A_num = state.n_a
		reply.A_value = state.n_v
		px.instances[args.Seq].n_p = args.PNum
	}
	return nil
}

//
// the application on this machine is done with
// all instances <= seq.
//
// see the comments for Min() for more explanation.
//
func (px *Paxos) Done(seq int) {
	// Your code here.
	px.mu.Lock()
	defer px.mu.Unlock()

	if seq > px.dones[px.me] {
		px.dones[px.me] = seq
	}
}

//
// the application wants to know the
// highest instance sequence known to
// this peer.
//
func (px *Paxos) Max() int {
	// Your code here.
	px.mu.Lock()
	defer px.mu.Unlock()

	max := 0
	for k, _ := range px.instances {
		if k > max {
			max = k
		}
	}
	return max
}

//
// Min() should return one more than the minimum among z_i,
// where z_i is the highest number ever passed
// to Done() on peer i. A peers z_i is -1 if it has
// never called Done().
//
// Paxos is required to have forgotten all information
// about any instances it knows that are < Min().
// The point is to free up memory in long-running
// Paxos-based servers.
//
// Paxos peers need to exchange their highest Done()
// arguments in order to implement Min(). These
// exchanges can be piggybacked on ordinary Paxos
// agreement protocol messages, so it is OK if one
// peers Min does not reflect another Peers Done()
// until after the next instance is agreed to.
//
// The fact that Min() is defined as a minimum over
// *all* Paxos peers means that Min() cannot increase until
// all peers have been heard from. So if a peer is dead
// or unreachable, other peers Min()s will not increase
// even if all reachable peers call Done. The reason for
// this is that when the unreachable peer comes back to
// life, it will need to catch up on instances that it
// missed -- the other peers therefor cannot forget these
// instances.
//
func (px *Paxos) Min() int {
	// You code here.  
	px.mu.Lock()
	defer px.mu.Unlock()

	min := px.dones[px.me]
	for i := range px.dones {
		if px.dones[i] < min {
			min = px.dones[i]
		}
	}

	for k, instance := range px.instances {
		if k > min {
			continue
		}
		if instance.state != Decided {
			continue
		}
		delete(px.instances, k)
	}

	return min + 1
}

//
// the application wants to know whether this
// peer thinks an instance has been decided,
// and if so what the agreed value is. Status()
// should just inspect the local peer state;
// it should not contact other Paxos peers.
//
func (px *Paxos) Status(seq int) (Fate, interface{}) {
	// Your code here.
	if seq < px.Min() {
		return Forgotten, nil
	}
	px.mu.Lock()
	defer px.mu.Unlock()

	instance, ok := px.instances[seq]
	if ok == false {
		return Pending, nil
	}

	return instance.state, instance.n_v
}



//
// tell the peer to shut itself down.
// for testing.
// please do not change these two functions.
//
func (px *Paxos) Kill() {
	atomic.StoreInt32(&px.dead, 1)
	if px.l != nil {
		px.l.Close()
	}
}

//
// has this peer been asked to shut down?
//
func (px *Paxos) isdead() bool {
	return atomic.LoadInt32(&px.dead) != 0
}

// please do not change these two functions.
func (px *Paxos) setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&px.unreliable, 1)
	} else {
		atomic.StoreInt32(&px.unreliable, 0)
	}
}

func (px *Paxos) isunreliable() bool {
	return atomic.LoadInt32(&px.unreliable) != 0
}

//
// the application wants to create a paxos peer.
// the ports of all the paxos peers (including this one)
// are in peers[]. this servers port is peers[me].
//
func Make(peers []string, me int, rpcs *rpc.Server) *Paxos {
	px := &Paxos{}
	px.peers = peers
	px.me = me


	// Your initialization code here.
	px.instances = make(map[int]*State)
	px.dones = make([]int, len(peers))
	for i := 0; i < len(peers); i++ {
		px.dones[i] = -1
	}

	if rpcs != nil {
		// caller will create socket &c
		rpcs.Register(px)
	} else {
		rpcs = rpc.NewServer()
		rpcs.Register(px)

		// prepare to receive connections from clients.
		// change "unix" to "tcp" to use over a network.
		os.Remove(peers[me]) // only needed for "unix"
		l, e := net.Listen("unix", peers[me])
		if e != nil {
			log.Fatal("listen error: ", e)
		}
		px.l = l

		// please do not change any of the following code,
		// or do anything to subvert it.

		// create a thread to accept RPC connections
		go func() {
			for px.isdead() == false {
				conn, err := px.l.Accept()
				if err == nil && px.isdead() == false {
					if px.isunreliable() && (rand.Int63()%1000) < 100 {
						// discard the request.
						conn.Close()
					} else if px.isunreliable() && (rand.Int63()%1000) < 200 {
						// process the request but force discard of reply.
						c1 := conn.(*net.UnixConn)
						f, _ := c1.File()
						err := syscall.Shutdown(int(f.Fd()), syscall.SHUT_WR)
						if err != nil {
							fmt.Printf("shutdown: %v\n", err)
						}
						atomic.AddInt32(&px.rpcCount, 1)
						go rpcs.ServeConn(conn)
					} else {
						atomic.AddInt32(&px.rpcCount, 1)
						go rpcs.ServeConn(conn)
					}
				} else if err == nil {
					conn.Close()
				}
				if err != nil && px.isdead() == false {
					fmt.Printf("Paxos(%v) accept: %v\n", me, err.Error())
				}
			}
		}()
	}


	return px
}
