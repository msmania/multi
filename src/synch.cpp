#include <windows.h>
#include "common.h"

Process::Process() : handle_(GetCurrentProcess()) {}

Process::Process(DWORD pid, DWORD desiredAccess) : handle_{} {
  handle_ = OpenProcess(desiredAccess, /*bInheritHandle*/FALSE, pid);
  if (!handle_) {
    Log(L"OpenProcess failed - %08lx\n", GetLastError());
  }
}

Process::~Process() {
  if (handle_) {
    CloseHandle(handle_);
  }
}

Process::operator bool() const {
  return !!handle_;
}

Process::operator HANDLE() const {return handle_; }

HANDLE Process::MoveHandleFrom(const Process &sourceProcess,
                               HANDLE handleToMove) const {
  HANDLE newHandle = nullptr;
  BOOL ok = DuplicateHandle(
    sourceProcess,
    handleToMove,
    *this,
    &newHandle,
    /*dwDesiredAccess*/0,
    /*bInheritHandle*/FALSE,
    DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  if (!ok) {
    Log(L"DuplicateHandle failed - %08lx\n", GetLastError());
  }
  return newHandle;
}

HANDLE Process::MoveHandleTo(HANDLE handleToMove,
                             const Process &targetProcess) const {
  HANDLE newHandle = nullptr;
  BOOL ok = DuplicateHandle(
    *this,
    handleToMove,
    targetProcess,
    &newHandle,
    /*dwDesiredAccess*/0,
    /*bInheritHandle*/FALSE,
    DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  if (!ok) {
    Log(L"DuplicateHandle failed - %08lx\n", GetLastError());
  }
  return newHandle;
}
