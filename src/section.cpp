#include <windows.h>
#include "common.h"

SharedMemory::MappedView::MappedView(const SharedMemory &section,
           DWORD desiredAccess,
           DWORD offset,
           DWORD size) : base_{} {
  base_ = MapViewOfFile(
    section,
    desiredAccess,
    /*dwFileOffsetHigh*/0,
    /*dwFileOffsetLow*/offset,
    /*dwNumberOfBytesToMap*/size);
  if (!base_) {
    Log(L"MapViewOfFile failed - %08lx\n", GetLastError());
  }
}

SharedMemory::MappedView::MappedView(MappedView &&other) : base_{} {
  std::swap(base_, other.base_);
}

SharedMemory::MappedView &SharedMemory::MappedView::operator=(
    MappedView &&other) {
  if (this != &other) {
    base_ = other.base_;
    other.base_ = nullptr;
  }
  return *this;
}

SharedMemory::MappedView::~MappedView() {
  if (base_) {
    UnmapViewOfFile(base_);
  }
}

SharedMemory::MappedView::operator bool() {
  return !!base_;
}

SharedMemory::SharedMemory(DWORD size, LPCWSTR name) : handle_{} {
  handle_ = CreateFileMapping(
    /*hFile*/INVALID_HANDLE_VALUE,
    /*lpFileMappingAttributes*/nullptr,
    /*flProtect*/PAGE_READWRITE,
    /*dwMaximumSizeHigh*/0,
    /*dwMaximumSizeLow*/size,
    /*lpName*/name);
  if (!handle_) {
    Log(L"CreateFileMapping failed - %08lx\n", GetLastError());
  }
}

SharedMemory::SharedMemory(HANDLE section) : handle_(section) {}

SharedMemory::~SharedMemory() {
  if (handle_) {
    CloseHandle(handle_);
  }
}

SharedMemory::operator bool() const {return !!handle_;}
SharedMemory::operator HANDLE() const {return handle_ ;}

SharedMemory::MappedView SharedMemory::Map(DWORD desiredAccess,
                             DWORD offset,
                             DWORD size) {
  return MappedView(*this, desiredAccess, offset, size);
}