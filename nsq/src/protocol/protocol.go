package protocol

import (
    "net"
)

type Protocol interface {
    IOLoop(conn net.Conn) error
}
