#include <memory>
#include <windows.h>
#include "basewindow.h"
#include "common.h"

class MainWindow : public BaseWindow<MainWindow> {
  SandboxClient client_;

  bool OnCreate() {
    if (!client_) {
      return false;
    }

    auto ntdll = NtDll::GetInstance();
    if (!ntdll) {
      return false;
    }

    ntdll->SetRpcClientWeakRef(&client_);
    if (!ntdll->Hook()) {
      return false;
    }

    return true;
  }

public:
  MainWindow(const std::wstring &endpointId) : client_(endpointId)
  {}

  LPCTSTR ClassName() const {
    return L"MainWindow";
  }

  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) {
    LRESULT ret = 0;
    switch (msg) {
    case WM_CREATE:
      if (!OnCreate()) {
        return -1;
      }
      break;
    case WM_DESTROY:
      client_.Shutdown();
      PostQuitMessage(0);
      break;
    default:
      ret = DefWindowProc(hwnd(), msg, w, l);
      break;
    }
    return ret;
  }
};

void ClientMain(const wchar_t *uuid) {
  if (auto p = std::make_unique<MainWindow>(uuid)) {
    if (p->Create(L"MainWindow Title",
                  WS_OVERLAPPEDWINDOW,
                  /*style_ex*/0,
                  CW_USEDEFAULT, 0,
                  486, 300)) {
      ShowWindow(p->hwnd(), SW_SHOW);
      MSG msg{};
      while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
}

HANDLE DoCreateProcess(const wchar_t *executable,
                     const std::wstring &command,
                     DWORD creationFlags,
                     LPSTARTUPINFO si) {
  HANDLE processHandle = nullptr;
  if (auto copied = new wchar_t[command.size() + 1]) {
    command.copy(copied, command.size());
    copied[command.size()] = 0;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcess(executable,
                       copied,
                       /*lpProcessAttributes*/nullptr,
                       /*lpThreadAttributes*/nullptr,
                       /*bInheritHandles*/FALSE,
                       creationFlags,
                       /*lpEnvironment*/nullptr,
                       /*lpCurrentDirectory*/nullptr,
                       si,
                       &pi)) {
      Log(L"CreateProcess failed - %08x\n", GetLastError());
      return nullptr;
    }

    processHandle = pi.hProcess;

    CloseHandle(pi.hThread);
    delete [] copied;
  }
  return processHandle;
}

const wchar_t Option_Child[] = L"--child=";

DWORD WINAPI RpcServerThread(LPVOID param) {
  if (auto server = reinterpret_cast<SandboxServer*>(param)) {
    server->StartAndWait();
  }
  return 0;
}

void ServerMain() {
  CodeIntegrityGuard cig;
  if (!cig) {
    return;
  }

  std::wstring uuidStr = GenerateUuidString();

  SandboxServer server(uuidStr);
  if (!server) {
    return;
  }

  Thread rpcThread(RpcServerThread, &server, /*creationFlags*/0);
  if (!rpcThread) {
    return;
  }

  wchar_t exe_name[MAX_PATH];
  if (!GetModuleFileName(nullptr, exe_name, MAX_PATH)) {
    Log(L"GetModuleFileName failed - %08lx\n", GetLastError());
    TerminateThread(rpcThread, 0);
    return;
  }

  std::wstring fullCommand = exe_name;
  fullCommand += L' ';
  fullCommand += Option_Child;
  fullCommand += uuidStr;

  STARTUPINFOEX si = {};
  si.StartupInfo.cb = sizeof(si);
  si.lpAttributeList = cig;
  auto process = DoCreateProcess(exe_name,
                                 fullCommand,
                                 EXTENDED_STARTUPINFO_PRESENT,
                                 &si.StartupInfo);
  if (!process) {
    TerminateThread(rpcThread, 0);
    return;
  }

  WaitForSingleObject(process, INFINITE);
  CloseHandle(process);

  if (WaitForSingleObject(rpcThread, 100) == WAIT_TIMEOUT) {
    TerminateThread(rpcThread, 0);
  }
}

class FileMapper final {
  HANDLE mSecionHandle;
  LPVOID mMappedView;

 public:
  FileMapper(LPCWSTR path) noexcept : mSecionHandle{}, mMappedView{} {
    HANDLE file = CreateFile(
        path,
        GENERIC_READ | GENERIC_EXECUTE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_ARCHIVE,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
      Log(L"CreateFile failed - %08x\n", GetLastError());
      return;
    }

    mSecionHandle = CreateFileMapping(
        file,
        nullptr,
        PAGE_EXECUTE_READ,
        0, 0,
        nullptr);
    if (!mSecionHandle) {
      Log(L"CreateFileMapping failed - %08x\n", GetLastError());
      CloseHandle(file);
      return;
    }

    CloseHandle(file);

    mMappedView = MapViewOfFileEx(
        mSecionHandle,
        FILE_MAP_EXECUTE,
        0, 0, 0,
        reinterpret_cast<uint8_t*>(::GetModuleHandle(nullptr)) + 0xB0000);
    if (!mMappedView) {
      Log(L"MapViewOfFile failed - %08x\n", GetLastError());
      return;
    }

    Log(L"Mapped onto %p\n", mMappedView);
  }

  FileMapper(FileMapper&& aOther)
      : mSecionHandle(aOther.mSecionHandle),
        mMappedView(aOther.mMappedView) {
    aOther.mSecionHandle = nullptr;
    aOther.mMappedView = nullptr;
  }

  FileMapper& operator=(FileMapper&& aOther) {
    if (this != &aOther) {
      mSecionHandle = aOther.mSecionHandle;
      aOther.mSecionHandle = nullptr;
      mMappedView = aOther.mMappedView;
      aOther.mMappedView = nullptr;
    }
    return *this;
  }

  FileMapper(const FileMapper&) = delete;
  FileMapper& operator=(const FileMapper&) = delete;

  ~FileMapper() {
    if (mMappedView) {
      if (!UnmapViewOfFile(mMappedView)) {
        Log(L"UnmapViewOfFile failed - %08x\n", GetLastError());
      }
    }

    if (mSecionHandle) {
      if (!CloseHandle(mSecionHandle)) {
        Log(L"CloseHandle(section) failed - %08x\n", GetLastError());
      }
    }
  }
};

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmdline, int) {
  wchar_t thisExe[MAX_PATH + 1] = {};
  if (GetModuleFileName(nullptr, thisExe, MAX_PATH)) {
    std::vector<FileMapper> v;
    for (int i = 0; i < 10; ++i) {
      v.emplace_back(thisExe);
    }
  }

  const auto args = get_args(cmdline);
  if (args.size() == 0) {
    ServerMain();
  }
  else if (args.size() >= 1
           && args[0].size() == 46
           && args[0].find(Option_Child) == 0) {
    ClientMain(args[0].c_str() + _countof(Option_Child) - 1);
  }
  return 0;
}
