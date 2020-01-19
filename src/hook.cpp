#include <functional>
#include <windows.h>
#include <detours.h>
#include "common.h"

static bool DetourTransaction(std::function<bool()> callback) {
  LONG status = DetourTransactionBegin();
  if (status != NO_ERROR) {
    Log(L"DetourTransactionBegin failed with %08lx\n", status);
    return status;
  }

  if (callback()) {
    status = DetourTransactionCommit();
    if (status != NO_ERROR) {
      Log(L"DetourTransactionCommit failed with %08lx\n", status);
    }
  }
  else {
    status = DetourTransactionAbort();
    if (status == NO_ERROR) {
      Log(L"Aborted transaction.\n");
    }
    else {
      Log(L"DetourTransactionAbort failed with %08lx\n", status);
    }
  }
  return status == NO_ERROR;
}

NtDll *NtDll::GetInstance() {
  static INIT_ONCE initOnce = INIT_ONCE_STATIC_INIT;

  PVOID param = nullptr;
  BOOL status = InitOnceExecuteOnce(
    &initOnce,
    [](PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) -> BOOL {
      static NtDll theInstance;
      if (Context) *Context = &theInstance;
      return TRUE;
    },
    /*Context*/nullptr,
    &param);

  if (!status) {
    return nullptr;
  }

  return reinterpret_cast<NtDll*>(param);
}

NTSTATUS NTAPI NtDll::detoured_NtCreateSection(PHANDLE SectionHandle,
                                               ACCESS_MASK DesiredAccess,
                                               PVOID ObjectAttributes,
                                               PLARGE_INTEGER MaximumSize,
                                               ULONG SectionPageProtection,
                                               ULONG AllocationAttributes,
                                               HANDLE FileHandle) {
  constexpr NTSTATUS STATUS_NOT_IMPLEMENTED = 0xC0000002L;

  if (auto ntdll = NtDll::GetInstance()) {
    return ntdll->NtCreateSection(SectionHandle,
                                  DesiredAccess,
                                  ObjectAttributes,
                                  MaximumSize,
                                  SectionPageProtection,
                                  AllocationAttributes,
                                  FileHandle);
  }
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NtDll::NtCreateSection(PHANDLE SectionHandle,
                                ACCESS_MASK DesiredAccess,
                                PVOID ObjectAttributes,
                                PLARGE_INTEGER MaximumSize,
                                ULONG SectionPageProtection,
                                ULONG AllocationAttributes,
                                HANDLE FileHandle) {
  Log(L"SH=%p DH=%08lx OA=%p MS=%p SPP=%08lx AA=%08lx FH=%08lx\n",
      SectionHandle,
      DesiredAccess,
      ObjectAttributes,
      MaximumSize,
      SectionPageProtection,
      AllocationAttributes,
      FileHandle);

  return NtCreateSection_(SectionHandle,
                          DesiredAccess,
                          ObjectAttributes,
                          MaximumSize,
                          SectionPageProtection,
                          AllocationAttributes,
                          FileHandle);
}

NtDll::NtDll()
  : module_(LoadLibrary(L"ntdll.dll")),
    NtCreateSection_(reinterpret_cast<decltype(&detoured_NtCreateSection)>(
      GetProcAddress(module_, "NtCreateSection")))
{}

NtDll::~NtDll() {
  if (module_) {
    FreeLibrary(module_);
  }
}

NtDll::operator bool() const {
  return !!module_ && !!NtCreateSection_;
}

bool NtDll::Hook() {
  static INIT_ONCE hookOnce = INIT_ONCE_STATIC_INIT;
  return !!InitOnceExecuteOnce(
    &hookOnce,
    [](PINIT_ONCE, PVOID, PVOID*) -> BOOL {
      auto ntdll = NtDll::GetInstance();
      if (!ntdll) return FALSE;

      return DetourTransaction(
        [ntdll]() {
          PDETOUR_TRAMPOLINE trampoline;
          PVOID realTarget, realDetour;
          LONG status = DetourAttachEx(
              reinterpret_cast<void**>(&ntdll->NtCreateSection_),
              NtDll::detoured_NtCreateSection,
              &trampoline,
              &realTarget,
              &realDetour);
          if (status == NO_ERROR) {
            Log(L"Detouring: %p --> %p (trampoline:%p)\n",
                realTarget,
                realDetour,
                trampoline);
          }
          else {
            Log(L"DetourAttach(%p, %p) failed with %08x\n",
                ntdll->NtCreateSection_,
                detoured_NtCreateSection,
                status);
          }
          return status == NO_ERROR;
        });
    },
    /*Context*/nullptr,
    /*Parameter*/nullptr
  );
}
