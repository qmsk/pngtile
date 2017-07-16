package pngtile

/*
#include "pngtile.h"
*/
import "C"
import (
	"fmt"
	"syscall"
)

type CallError struct {
	call string
	ret  C.int
}

func (err CallError) String() string {
	return C.GoString(C.pt_strerror(err.ret))
}

type SysCallError struct {
	CallError
	errno syscall.Errno
}

type CallErrorError struct {
	CallError
	err error
}

func makeError(call string, ret C.int, err error) error {
	if err == nil {
		return CallError{call, ret}
	} else if errno, ok := err.(syscall.Errno); ok {
		return SysCallError{CallError{call, ret}, errno}
	} else {
		return CallErrorError{CallError{call, ret}, err}
	}
}

func (err CallError) Error() string {
	return fmt.Sprintf("%s: %s", err.call, err.String())
}

func (err SysCallError) Error() string {
	return fmt.Sprintf("%s: %s: %s", err.call, err.String(), err.errno.Error())
}

func (err CallErrorError) Error() string {
	return fmt.Sprintf("%s: %s: %s", err.call, err.String(), err.err.Error())
}
