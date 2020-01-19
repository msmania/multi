#include <windows.h>
#include "common.h"

CodeIntegrityGuard::CodeIntegrityGuard()
  : attributeList_(nullptr),
    policy_(PROCESS_CREATION_MITIGATION_POLICY_BLOCK_NON_MICROSOFT_BINARIES_ALWAYS_ON) {
  SIZE_T AttributeListSize;
  if (InitializeProcThreadAttributeList(nullptr,
                                        /*dwAttributeCount*/1,
                                        /*dwFlags*/0,
                                        &AttributeListSize)
      || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return;
  }

  if (!attributeListBlob_.Alloc(AttributeListSize)) return;

  attributeList_ = attributeListBlob_.As<_PROC_THREAD_ATTRIBUTE_LIST>();
  if (!InitializeProcThreadAttributeList(attributeList_,
                                         /*dwAttributeCount*/1,
                                         /*dwFlags*/0,
                                         &AttributeListSize)) {
    attributeList_ = nullptr;
    Log(L"InitializeProcThreadAttributeList failed - %08x\n", GetLastError());
    return;
  }


  if (!UpdateProcThreadAttribute(attributeList_,
                                 /*dwFlags*/0,
                                 PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY,
                                 &policy_,
                                 sizeof(policy_),
                                 /*lpPreviousValue*/nullptr,
                                 /*lpReturnSize*/nullptr)) {
    Log(L"UpdateProcThreadAttribute failed - %08x\n", GetLastError());
  }
}

CodeIntegrityGuard::~CodeIntegrityGuard() {
  if (attributeList_) {
    DeleteProcThreadAttributeList(attributeList_);
  }
}

CodeIntegrityGuard::operator LPPROC_THREAD_ATTRIBUTE_LIST() {
  return attributeList_;
}
