#pragma once

#include <vector>
#include <string>
#include <strsafe.h>
#include "blob.h"

extern RPC_WSTR RPC_Protocol;

void Log(LPCTSTR format, ...);
std::wstring GenerateUuidString();

template <typename CharType>
std::vector<std::basic_string<CharType>> get_args(const CharType *args) {
  std::vector<std::basic_string<CharType>> string_array;
  const CharType *prev, *p;
  prev = p = args;
  while (*p) {
    if (*p == ' ') {
      if (p > prev)
        string_array.emplace_back(args, prev - args, p - prev);
      prev = p + 1;
    }
    ++p;
  }
  if (p > prev)
    string_array.emplace_back(args, prev - args, p - prev);
  return string_array;
}

template<size_t N>
bool GenerateEndpointName(wchar_t (&endpoint)[N], const wchar_t *endpointId) {
  RPC_WSTR Prefix = L"\\pipe\\sandbox-";
  HRESULT hr;

  size_t remaining = N;
  wchar_t *p = endpoint;
  hr = StringCchCopyEx(p, remaining, Prefix, &p, &remaining, 0);
  if (FAILED(hr)) {
    Log(L"StringCchCopyEx#1 failed - %08lx\n", hr);
    return false;
  }
  hr = StringCchCopyEx(p, remaining, endpointId, &p, &remaining, 0);
  if (FAILED(hr)) {
    Log(L"StringCchCopyEx#2 failed - %08lx\n", hr);
    return false;
  }
  return true;
}

class SandboxClient final {
  RPC_BINDING_HANDLE binding_;

  template<typename... Args>
  ULONG CallMethod(void (*callback)(RPC_BINDING_HANDLE binding, Args...),
                   Args... args) {
    ULONG status = 0;
    RpcTryExcept {
      callback(binding_, args...);
    }
    RpcExcept(1) {
      status = RpcExceptionCode();
      Log(L"CallMethod failed - %08lx\n", status);
    }
    RpcEndExcept
    return status;
  }

public:
  SandboxClient(const std::wstring &endpointId);
  ~SandboxClient();

  operator bool() const;

  ULONG NtCreateSection(long fileHandle, long* sectionHandle);
  ULONG Shutdown();
};

class SandboxServer final {
  bool initialized_;

public:
  SandboxServer(const std::wstring &endpointId);

  operator bool() const;

  void StartAndWait() const;
};

class CodeIntegrityGuard final {
  Blob attributeListBlob_;
  LPPROC_THREAD_ATTRIBUTE_LIST attributeList_;
  DWORD64 policy_;

public:
  CodeIntegrityGuard();
  ~CodeIntegrityGuard();

  operator LPPROC_THREAD_ATTRIBUTE_LIST();
};