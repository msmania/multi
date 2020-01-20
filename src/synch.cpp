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

HANDLE Process::DuplicateHandleFrom(const Process &sourceProcess,
                                    HANDLE handleToDuplicate,
                                    bool closeSourceHandle) const {
  HANDLE newHandle = nullptr;
  DWORD flags = DUPLICATE_SAME_ACCESS;
  if (closeSourceHandle) flags |= DUPLICATE_CLOSE_SOURCE;
  BOOL ok = DuplicateHandle(sourceProcess,
                            handleToDuplicate,
                            *this,
                            &newHandle,
                            /*dwDesiredAccess*/0,
                            /*bInheritHandle*/FALSE,
                            flags);
  if (!ok) {
    Log(L"DuplicateHandle failed - %08lx\n", GetLastError());
  }
  return newHandle;
}

HANDLE Process::DuplicateHandleTo(HANDLE handleToDuplicate,
                                  const Process &targetProcess,
                                  bool closeSourceHandle) const {
  HANDLE newHandle = nullptr;
  DWORD flags = DUPLICATE_SAME_ACCESS;
  if (closeSourceHandle) flags |= DUPLICATE_CLOSE_SOURCE;
  BOOL ok = DuplicateHandle(*this,
                            handleToDuplicate,
                            targetProcess,
                            &newHandle,
                            /*dwDesiredAccess*/0,
                            /*bInheritHandle*/FALSE,
                            flags);
  if (!ok) {
    Log(L"DuplicateHandle failed - %08lx\n", GetLastError());
  }
  return newHandle;
}

Thread::Thread(LPTHREAD_START_ROUTINE threadStart,
               PVOID param,
               DWORD creationFlags)
    : handle_(CreateThread(/*lpThreadAttributes*/nullptr,
                           /*dwStackSize*/0,
                           threadStart,
                           param,
                           creationFlags,
                           /*lpThreadId*/nullptr)) {
  if (!handle_) {
    Log(L"CreateThread failed - %08lx\n", GetLastError());
  }
}

Thread::~Thread() {
  if (handle_) {
    WaitForSingleObject(handle_, INFINITE);
    CloseHandle(handle_);
  }
}

Thread::operator bool() const {
  return !!handle_;
}

Thread::operator HANDLE() const {
  return handle_;
}
