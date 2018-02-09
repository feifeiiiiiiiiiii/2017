package main

import (
    "nsqd"
    "log"
    "syscall"
    "github.com/judwhite/go-svc/svc"
)

// program implements svc.Service
type program struct {
    nsqd *nsqd.NSQD
}


func main() {
    prg := &program{}

	// Call svc.Run to start your program/service.
    if err := svc.Run(prg, syscall.SIGINT, syscall.SIGTERM); err != nil {
		log.Fatal(err)
	}

}

func (p *program) Init(env svc.Environment) error {
    log.Printf("is win service? %v\n", env.IsWindowsService())
    return nil
}

func (p *program) Start() error {
    log.Println("start")
    nsqd := nsqd.New()
    nsqd.Main()
    p.nsqd = nsqd
    return nil
}


func (p *program) Stop() error {
    log.Println("stop")
    return nil
}

