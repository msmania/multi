#include <windows.h>
#include "common.h"
#include "sandbox.h"

SandboxClient::SandboxClient(const std::wstring &endpointId) : binding_{} {
  wchar_t endpointBuffer[100];
  GenerateEndpointName(endpointBuffer, endpointId.c_str());

  RPC_WSTR bindStr;
  RPC_STATUS status = RpcStringBindingCompose(
    /*ObjUuid*/nullptr,
    /*ProtSeq*/RPC_Protocol,
    /*NetworkAddr*/nullptr,
    /*Endpoint*/endpointBuffer,
    /*Options*/nullptr,
    /*StringBinding*/&bindStr);
  if (status) {
    Log(L"RpcStringBindingCompose failed - %08lx\n", status);
  }

  status = RpcBindingFromStringBinding(bindStr, &binding_);
  if (status) {
    Log(L"RpcBindingFromStringBinding failed - %08lx\n", status);
  }

  RpcStringFree(&bindStr);
}

SandboxClient::~SandboxClient() {
  if (binding_) {
    RpcBindingFree(&binding_);
  }
}

SandboxClient::operator bool() const {
  return !!binding_;
}

ULONG SandboxClient::NtCreateSection(long fileHandle, long* sectionHandle) {
  return CallMethod(c_NtCreateSection, fileHandle, sectionHandle);
}

ULONG SandboxClient::Shutdown() {
  return CallMethod(c_Shutdown);
}
