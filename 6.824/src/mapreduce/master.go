package mapreduce

import "container/list"
import "fmt"


type WorkerInfo struct {
	address string
	// You can add definitions here.
}


// Clean up all workers by sending a Shutdown RPC to each one of them Collect
// the number of jobs each work has performed.
func (mr *MapReduce) KillWorkers() *list.List {
	l := list.New()
	for _, w := range mr.Workers {
		DPrintf("DoWork: shutdown %s\n", w.address)
		args := &ShutdownArgs{}
		var reply ShutdownReply
		ok := call(w.address, "Worker.Shutdown", args, &reply)
		if ok == false {
			fmt.Printf("DoWork: RPC %s shutdown error\n", w.address)
		} else {
			l.PushBack(reply.Njobs)
		}
	}
	return l
}

func (mr *MapReduce)doTask(worker string, index int, jobType JobType, numPhase int) bool {
	var job DoJobArgs
	var reply DoJobReply

	job.File = mr.file
	job.Operation = jobType
	job.JobNumber = index
	job.NumOtherPhase = numPhase

	return call(worker, "Worker.DoJob", job, &reply)
}

func (mr *MapReduce) RunMaster() *list.List {
	// Your code here

	// 启动map worker
	var mapChan = make(chan int, mr.nMap)
	for i := 0; i < mr.nMap; i++ {
		go func(idx int) {
			for {
				var worker string
				var ok bool = false

				select {
				case worker = <- mr.registerChannel:
					ok = mr.doTask(worker, idx, Map, mr.nReduce)
				case worker = <- mr.idleChannel:
					ok = mr.doTask(worker, idx, Map, mr.nReduce)
				}
				if ok {
					mapChan <- idx
					mr.idleChannel <- worker
					return
				}
			}
		}(i)
	}
	for i := 0; i < mr.nMap; i++{
		<- mapChan
	}

    close(mapChan)

	// 启动reduce worker
	var reduceChan = make(chan int, mr.nReduce)
	for i := 0; i < mr.nReduce; i++ {
		go func(idx int) {
			for {
				var worker string
				var ok bool = false

				select {
				case worker = <- mr.registerChannel:
					ok = mr.doTask(worker, idx, Reduce, mr.nMap)
				case worker = <- mr.idleChannel:
					ok = mr.doTask(worker, idx, Reduce, mr.nMap)
				}
				if ok {
					reduceChan <- idx
					mr.idleChannel <- worker
					return
				}
			}
		}(i)
	}
	for i := 0; i < mr.nReduce; i++{
		<- reduceChan
	}
    close(reduceChan)

		
	return mr.KillWorkers()
}
