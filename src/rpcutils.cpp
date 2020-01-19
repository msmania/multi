#include <windows.h>

void __RPC_FAR *__RPC_USER midl_user_allocate(size_t len) {
  return HeapAlloc(GetProcessHeap(), 0, len);
}
 
void __RPC_USER midl_user_free(void __RPC_FAR * ptr) {
  HeapFree(GetProcessHeap(), 0, ptr);
}
