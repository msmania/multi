#include <windows.h>
#include "common.h"
#include "sandbox.h"

RPC_WSTR RPC_Protocol = L"ncacn_np";

void s_GetServerPid(
    /* [in] */ handle_t IDL_handle,
    /* [out] */ unsigned long *serverPid) {
  if (serverPid) {
    *serverPid = GetCurrentProcessId();
  }
}

void s_NtCreateSection( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ unsigned long clientPid,
    /* [in] */ unsigned long fileHandle,
    /* [in] */ unsigned long desiredAccess,
    /* [in] */ unsigned long sectionPageProtection,
    /* [in] */ unsigned long allocationAttributes,
    /* [out] */ unsigned long *sectionHandle,
    /* [out] */ unsigned long *status) {
  *sectionHandle = fileHandle + 0x42;
}

void s_Shutdown( 
    /* [in] */ handle_t IDL_handle) {
  RPC_STATUS status = RpcMgmtStopServerListening(/*Binding*/nullptr);
  if (status) {
    Log(L"RpcMgmtStopServerListening failed - %08lx\n", status);
  }

  status = RpcServerUnregisterIf(
    /*IfSpec*/nullptr,
    /*MgrTypeUuid*/nullptr,
    /*WaitForCallsToComplete*/FALSE);
  if (status) {
    Log(L"RpcServerUnregisterIf failed - %08lx\n", status);
  }
}

SandboxServer::SandboxServer(const std::wstring &endpointId)
    : initialized_(false) {

  wchar_t endpointBuffer[100];
  GenerateEndpointName(endpointBuffer, endpointId.c_str());

  RPC_STATUS status = RpcServerUseProtseqEp(
    /*Protseq*/RPC_Protocol,
    /*MaxCalls*/RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
    /*Endpoint*/endpointBuffer,
    /*SecurityDescriptor*/nullptr);
  if (status) {
    Log(L"RpcServerUseProtseqEp failed - %08lx\n", status);
  }

  status = RpcServerRegisterIf(
    s_sandbox_v1_0_s_ifspec,
    /*MgrTypeUuid*/nullptr,
    /*MgrEpv*/nullptr);
  if (status) {
    Log(L"RpcServerRegisterIf failed - %08lx\n", status);
  }

  initialized_ = true;
}

SandboxServer::operator bool() const {
  return initialized_;
}

void SandboxServer::StartAndWait() const {
  RPC_STATUS status = RpcServerListen(
    /*MinimumCallThreads*/1,
    /*MaxCalls*/RPC_C_LISTEN_MAX_CALLS_DEFAULT,
    /*DontWait*/0);
  if (status) {
    Log(L"RpcServerListen failed - %08lx\n", status);
  }
}
