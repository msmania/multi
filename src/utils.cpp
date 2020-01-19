#include <windows.h>
#include "common.h"

void Log(LPCTSTR format, ...) {
  TCHAR linebuf[1024];
  va_list v;
  va_start(v, format);
  StringCbVPrintf(linebuf, sizeof(linebuf), format, v);
  va_end(v);
  OutputDebugString(linebuf);
}

void LogPrint(LPCTSTR format, ...) {
  va_list v;
  va_start(v, format);
  vwprintf(format, v);
  va_end(v);
}

std::wstring GenerateUuidString() {
  UUID uuid;
  RPC_STATUS status = UuidCreate(&uuid);
  if (status) {
    Log(L"UuidCreate failed - %08lx\n", status);
    return L"";
  }

  wchar_t uuidStr[39];
  StringCbPrintf(uuidStr, sizeof(uuidStr),
                 L"{%08lx-%04lx-%04lx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 uuid.Data1,
                 uuid.Data2,
                 uuid.Data3,
                 uuid.Data4[0],
                 uuid.Data4[1],
                 uuid.Data4[2],
                 uuid.Data4[3],
                 uuid.Data4[4],
                 uuid.Data4[5],
                 uuid.Data4[6],
                 uuid.Data4[7]);
  return uuidStr;
}
