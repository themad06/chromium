// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package system

//#include "mojo/public/c/system/main.h"
//#include "mojo/public/platform/native/system_thunks.h"
import "C"
import (
	"math"
	"unsafe"
)

// Go equivalent definitions of the various system types defined in Mojo.
// mojo/public/c/system/types.h
// mojo/public/c/system/data_pipe.h
// mojo/public/c/system/message_pipe.h
//
type MojoTimeTicks int64
type MojoHandle uint32
type MojoResult int32
type MojoDeadline uint64
type MojoHandleSignals uint32
type MojoWriteMessageFlags uint32
type MojoReadMessageFlags uint32
type MojoWriteDataFlags uint32
type MojoReadDataFlags uint32
type MojoCreateDataPipeOptionsFlags uint32
type MojoCreateMessagePipeOptionsFlags uint32
type MojoCreateSharedBufferOptionsFlags uint32
type MojoDuplicateBufferHandleOptionsFlags uint32
type MojoMapBufferFlags uint32

const (
	MOJO_DEADLINE_INDEFINITE        MojoDeadline = math.MaxUint64
	MOJO_HANDLE_INVALID             MojoHandle   = 0
	MOJO_RESULT_OK                  MojoResult   = 0
	MOJO_RESULT_CANCELLED                        = -1
	MOJO_RESULT_UNKNOWN                          = -2
	MOJO_RESULT_INVALID_ARGUMENT                 = -3
	MOJO_RESULT_DEADLINE_EXCEEDED                = -4
	MOJO_RESULT_NOT_FOUND                        = -5
	MOJO_RESULT_ALREADY_EXISTS                   = -6
	MOJO_RESULT_PERMISSION_DENIED                = -7
	MOJO_RESULT_RESOURCE_EXHAUSTED               = -8
	MOJO_RESULT_FAILED_PRECONDITION              = -9
	MOJO_RESULT_ABORTED                          = -10
	MOJO_RESULT_OUT_OF_RANGE                     = -11
	MOJO_RESULT_UNIMPLEMENTED                    = -12
	MOJO_RESULT_INTERNAL                         = -13
	MOJO_RESULT_UNAVAILABLE                      = -14
	MOJO_RESULT_DATA_LOSS                        = -15
	MOJO_RESULT_BUSY                             = -16
	MOJO_RESULT_SHOULD_WAIT                      = -17

	MOJO_HANDLE_SIGNAL_NONE        MojoHandleSignals = 0
	MOJO_HANDLE_SIGNAL_READABLE                      = 1 << 0
	MOJO_HANDLE_SIGNAL_WRITABLE                      = 1 << 1
	MOJO_HANDLE_SIGNAL_PEER_CLOSED                   = 1 << 2

	MOJO_WRITE_MESSAGE_FLAG_NONE       MojoWriteMessageFlags = 0
	MOJO_READ_MESSAGE_FLAG_NONE        MojoReadMessageFlags  = 0
	MOJO_READ_MESSAGE_FLAG_MAY_DISCARD                       = 1 << 0

	MOJO_READ_DATA_FLAG_NONE         MojoReadDataFlags  = 0
	MOJO_READ_DATA_FLAG_ALL_OR_NONE                     = 1 << 0
	MOJO_READ_DATA_FLAG_DISCARD                         = 1 << 1
	MOJO_READ_DATA_FLAG_QUERY                           = 1 << 2
	MOJO_READ_DATA_FLAG_PEEK                            = 1 << 3
	MOJO_WRITE_DATA_FLAG_NONE        MojoWriteDataFlags = 0
	MOJO_WRITE_DATA_FLAG_ALL_OR_NONE MojoWriteDataFlags = 1 << 0

	MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE        MojoCreateDataPipeOptionsFlags    = 0
	MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_MAY_DISCARD                                   = 1 << 0
	MOJO_CREATE_MESSAGE_PIPE_OPTIONS_FLAG_NONE     MojoCreateMessagePipeOptionsFlags = 0

	MOJO_CREATE_SHARED_BUFFER_OPTIONS_FLAG_NONE    MojoCreateSharedBufferOptionsFlags    = 0
	MOJO_DUPLICATE_BUFFER_HANDLE_OPTIONS_FLAG_NONE MojoDuplicateBufferHandleOptionsFlags = 0
	MOJO_MAP_BUFFER_FLAG_NONE                      MojoMapBufferFlags                    = 0
)

// IsReadable returns true iff the |MOJO_HANDLE_SIGNAL_READABLE| bit is set.
func (m MojoHandleSignals) IsReadable() bool {
	return (m & MOJO_HANDLE_SIGNAL_READABLE) != 0
}

// IsWritable returns true iff the |MOJO_HANDLE_SIGNAL_WRITABLE| bit is set.
func (m MojoHandleSignals) IsWritable() bool {
	return (m & MOJO_HANDLE_SIGNAL_WRITABLE) != 0
}

// IsClosed returns true iff the |MOJO_HANDLE_SIGNAL_PEER_CLOSED| bit is set.
func (m MojoHandleSignals) IsClosed() bool {
	return (m & MOJO_HANDLE_SIGNAL_PEER_CLOSED) != 0
}

// MojoHandleSignalsState is a struct returned by wait functions to indicate
// the signaling state of handles.
type MojoHandleSignalsState struct {
	// Signals that were satisfied at some time before the call returned.
	SatisfiedSignals MojoHandleSignals
	// Signals that are possible to satisfy. For example, if the return value
	// was |MOJO_RESULT_FAILED_PRECONDITION|, you can use this field to
	// determine which, if any, of the signals can still be satisfied.
	SatisfiableSignals MojoHandleSignals
}

func (cState *C.struct_MojoHandleSignalsState) goValue() MojoHandleSignalsState {
	return MojoHandleSignalsState{
		MojoHandleSignals(cState.satisfied_signals),
		MojoHandleSignals(cState.satisfiable_signals),
	}
}

// DataPipeOptions is used to specify creation parameters for a data pipe.
type DataPipeOptions struct {
	flags MojoCreateDataPipeOptionsFlags
	// The size of an element in bytes. All transactions and buffers will
	// be an integral number of elements.
	elemSize uint32
	// The capacity of the data pipe in bytes. Must be a multiple of elemSize.
	capacity uint32
}

func (opts *DataPipeOptions) cValue(cOpts *C.struct_MojoCreateDataPipeOptions) *C.struct_MojoCreateDataPipeOptions {
	if opts == nil {
		return nil
	}
	*cOpts = C.struct_MojoCreateDataPipeOptions{
		C.uint32_t(unsafe.Sizeof(*cOpts)),
		opts.flags.cValue(),
		C.uint32_t(opts.elemSize),
		C.uint32_t(opts.capacity),
	}
	return cOpts
}

// MessagePipeOptions is used to specify creation parameters for a message pipe.
type MessagePipeOptions struct {
	flags MojoCreateMessagePipeOptionsFlags
}

func (opts *MessagePipeOptions) cValue(cOpts *C.struct_MojoCreateMessagePipeOptions) *C.struct_MojoCreateMessagePipeOptions {
	if opts == nil {
		return nil
	}
	*cOpts = C.struct_MojoCreateMessagePipeOptions{
		C.uint32_t(unsafe.Sizeof(*cOpts)),
		opts.flags.cValue(),
	}
	return cOpts
}

// SharedBufferOptions is used to specify creation parameters for a
// shared buffer.
type SharedBufferOptions struct {
	flags MojoCreateSharedBufferOptionsFlags
}

func (opts *SharedBufferOptions) cValue(cOpts *C.struct_MojoCreateSharedBufferOptions) *C.struct_MojoCreateSharedBufferOptions {
	if opts == nil {
		return nil
	}
	*cOpts = C.struct_MojoCreateSharedBufferOptions{
		C.uint32_t(unsafe.Sizeof(*cOpts)),
		opts.flags.cValue(),
	}
	return cOpts
}

// DuplicateBufferHandleOptions is used to specify parameters in
// duplicating access to a shared buffer.
type DuplicateBufferHandleOptions struct {
	flags MojoDuplicateBufferHandleOptionsFlags
}

func (opts *DuplicateBufferHandleOptions) cValue(cOpts *C.struct_MojoDuplicateBufferHandleOptions) *C.struct_MojoDuplicateBufferHandleOptions {
	if opts == nil {
		return nil
	}
	*cOpts = C.struct_MojoDuplicateBufferHandleOptions{
		C.uint32_t(unsafe.Sizeof(*cOpts)),
		opts.flags.cValue(),
	}
	return cOpts
}

// Convenience functions to convert Go types to their equivalent C types.
func (m MojoHandle) cValue() C.MojoHandle {
	return C.MojoHandle(m)
}

func (m MojoDeadline) cValue() C.MojoDeadline {
	return C.MojoDeadline(m)
}

func (m MojoHandleSignals) cValue() C.MojoHandleSignals {
	return C.MojoHandleSignals(m)
}

func (m MojoWriteMessageFlags) cValue() C.MojoWriteMessageFlags {
	return C.MojoWriteMessageFlags(m)
}

func (m MojoReadMessageFlags) cValue() C.MojoReadMessageFlags {
	return C.MojoReadMessageFlags(m)
}

func (m MojoWriteDataFlags) cValue() C.MojoWriteDataFlags {
	return C.MojoWriteDataFlags(m)
}

func (m MojoReadDataFlags) cValue() C.MojoReadDataFlags {
	return C.MojoReadDataFlags(m)
}

func (m MojoCreateDataPipeOptionsFlags) cValue() C.MojoCreateDataPipeOptionsFlags {
	return C.MojoCreateDataPipeOptionsFlags(m)
}

func (m MojoCreateMessagePipeOptionsFlags) cValue() C.MojoCreateMessagePipeOptionsFlags {
	return C.MojoCreateMessagePipeOptionsFlags(m)
}

func (m MojoCreateSharedBufferOptionsFlags) cValue() C.MojoCreateSharedBufferOptionsFlags {
	return C.MojoCreateSharedBufferOptionsFlags(m)
}

func (m MojoDuplicateBufferHandleOptionsFlags) cValue() C.MojoDuplicateBufferHandleOptionsFlags {
	return C.MojoDuplicateBufferHandleOptionsFlags(m)
}

func (m MojoMapBufferFlags) cValue() C.MojoMapBufferFlags {
	return C.MojoMapBufferFlags(m)
}
