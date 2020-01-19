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

    return true;
  }

  void OnLButtonDown() {
    long a = 1, b = 0;
    client_.NtCreateSection(a, &b);
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
    case WM_LBUTTONDOWN:
      OnLButtonDown();
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

void ServerMain() {
  CodeIntegrityGuard cig;
  if (!cig) {
    return;
  }

  wchar_t exe_name[MAX_PATH];
  if (!GetModuleFileName(nullptr, exe_name, MAX_PATH)) {
    Log(L"GetModuleFileName failed - %08lx\n", GetLastError());
    return;
  }

  std::wstring uuidStr = GenerateUuidString();

  SandboxServer server(uuidStr);
  if (!server) {
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
    return;
  }

  server.StartAndWait();

  WaitForSingleObject(process, INFINITE);
  CloseHandle(process);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmdline, int) {
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
