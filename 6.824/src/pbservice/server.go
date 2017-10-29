package pbservice

import "net"
import "fmt"
import "net/rpc"
import "log"
import "time"
import "viewservice"
import "sync"
import "sync/atomic"
import "os"
import "syscall"
import "math/rand"
import "errors"



type PBServer struct {
	mu         sync.Mutex
	l          net.Listener
	dead       int32 // for testing
	unreliable int32 // for testing
	me         string
	vs         *viewservice.Clerk
	// Your declarations here.

	db 		   	map[string]string
	view		viewservice.View
}


func (pb *PBServer) Get(args *GetArgs, reply *GetReply) error {

	// Your code here.

	pb.mu.Lock()
	defer pb.mu.Unlock()

	if !pb.isPrimary() {
		reply.Err = ErrWrongServer
		return nil
	}
	value := pb.db[args.Key]
	if value != "" {
		reply.Value = value
		reply.Err = OK
	} else {
		reply.Value = ""
		reply.Err = ErrNoKey
	}
	return nil
}


func (pb *PBServer) PutAppend(args *PutAppendArgs, reply *PutAppendReply) error {
	
	// Your code here.
	pb.mu.Lock()
	defer pb.mu.Unlock()

	if !pb.isPrimary() {
		reply.Err = ErrWrongServer
		return nil
	}

	var key = "uid_" + pb.me + "_" + args.Uid
	if pb.db[key] != "" {
		return nil
	}

	pb.db[key] = args.Uid

	value := pb.db[args.Key]

	if args.Op == "Append" {
		pb.db[args.Key] = value + args.Value
		reply.Err = OK
	} else if args.Op == "Put" {
		pb.db[args.Key] = args.Value
		reply.Err = OK		
	}

	// 同步数据到backup上
	pb.Forward(&ForwardArgs{Content: pb.db})
	
	return nil
}

func (pb *PBServer) isPrimary() bool {
	return pb.view.Primary == pb.me
}

func (pb *PBServer) hasBackup() bool {
	return pb.view.Backup != ""
}


//
// ping the viewserver periodically.
// if view changed:
//   transition to new view.
//   manage transfer of state from primary to new backup.
//
func (pb *PBServer) tick() {

	// Your code here.
	pb.mu.Lock()
	defer pb.mu.Unlock()

	view, _ := pb.vs.Ping(pb.view.Viewnum)
	
	// primary 发生改变, 发送primary‘s db到所有的backup上
	if pb.isPrimary() && view.Backup != pb.view.Backup && view.Backup != "" {
		pb.Forward(&ForwardArgs{Content: pb.db})
	}
	pb.view = view	
}

func (pb *PBServer) Forward(args *ForwardArgs) error {
	if !pb.hasBackup() {
		return nil
	}
	var reply ForwardReply
	ok := call(pb.view.Backup, "PBServer.ProcessForward", args, &reply)
	if !ok {
		return errors.New("[Foward] failed to forward put")
	}
	return nil
}

func (pb *PBServer) ProcessForward(args *ForwardArgs, reply *ForwardReply) error {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	for key,value := range args.Content{
		pb.db[key] = value
	}
	return nil
}


// tell the server to shut itself down.
// please do not change these two functions.
func (pb *PBServer) kill() {
	atomic.StoreInt32(&pb.dead, 1)
	pb.l.Close()
}

// call this to find out if the server is dead.
func (pb *PBServer) isdead() bool {
	return atomic.LoadInt32(&pb.dead) != 0
}

// please do not change these two functions.
func (pb *PBServer) setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&pb.unreliable, 1)
	} else {
		atomic.StoreInt32(&pb.unreliable, 0)
	}
}

func (pb *PBServer) isunreliable() bool {
	return atomic.LoadInt32(&pb.unreliable) != 0
}


func StartServer(vshost string, me string) *PBServer {
	pb := new(PBServer)
	pb.me = me
	pb.vs = viewservice.MakeClerk(me, vshost)
	// Your pb.* initializations here.

	pb.db = make(map[string]string)
	pb.view = viewservice.View{0, "", ""}

	rpcs := rpc.NewServer()
	rpcs.Register(pb)

	os.Remove(pb.me)
	l, e := net.Listen("unix", pb.me)
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	pb.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for pb.isdead() == false {
			conn, err := pb.l.Accept()
			if err == nil && pb.isdead() == false {
				if pb.isunreliable() && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if pb.isunreliable() && (rand.Int63()%1000) < 200 {
					// process the request but force discard of reply.
					c1 := conn.(*net.UnixConn)
					f, _ := c1.File()
					err := syscall.Shutdown(int(f.Fd()), syscall.SHUT_WR)
					if err != nil {
						fmt.Printf("shutdown: %v\n", err)
					}
					go rpcs.ServeConn(conn)
				} else {
					go rpcs.ServeConn(conn)
				}
			} else if err == nil {
				conn.Close()
			}
			if err != nil && pb.isdead() == false {
				fmt.Printf("PBServer(%v) accept: %v\n", me, err.Error())
				pb.kill()
			}
		}
	}()

	go func() {
		for pb.isdead() == false {
			pb.tick()
			time.Sleep(viewservice.PingInterval)
		}
	}()

	return pb
}
