#include <windows.h>
#include "common.h"
#include "sandbox.h"

SandboxClient::SandboxClient(const std::wstring &endpointId)
  : binding_{}, serverPid_{} {
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
    return;
  }

  status = RpcBindingFromStringBinding(bindStr, &binding_);
  if (status) {
    Log(L"RpcBindingFromStringBinding failed - %08lx\n", status);
    RpcStringFree(&bindStr);
    return;
  }
  RpcStringFree(&bindStr);

  if (CallMethod(c_GetServerPid, &serverPid_)) {
    RpcBindingFree(&binding_);
    binding_ = nullptr;
  }
}

SandboxClient::~SandboxClient() {
  if (binding_) {
    RpcBindingFree(&binding_);
  }
}

SandboxClient::operator bool() const {
  return !!binding_;
}

DWORD SandboxClient::GetServerPid() const {
  return serverPid_;
}

ULONG SandboxClient::NtCreateSection(unsigned long fileHandle,
                                     unsigned long desiredAccess,
                                     unsigned long sectionPageProtection,
                                     unsigned long allocationAttributes,
                                     unsigned long* sectionHandle,
                                     unsigned long* status) {
  return CallMethod(c_NtCreateSection,
                    GetCurrentProcessId(),
                    fileHandle,
                    desiredAccess,
                    sectionPageProtection,
                    allocationAttributes,
                    sectionHandle,
                    status);
}

ULONG SandboxClient::Shutdown() {
  return CallMethod(c_Shutdown);
}
