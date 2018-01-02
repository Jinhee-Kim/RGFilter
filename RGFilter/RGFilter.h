
#pragma once

#include <ntifs.h>
//#include <ntddk.h>
//#include <WinDef.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define _FN_	__FUNCTION__ // 함수명
#define _LN_	__LINE__ // 소스 파일의 줄 번호

#define NT_DEVICE_NAME			L"\\Device\\RGFilter"
#define DOS_DEVICES_LINK_NAME	L"\\DosDevices\\RGFilter"

#define REGISTRY_POOL_TAG 'RGF'

typedef unsigned long long QWORD;

#ifdef __cplusplus
EXTERN_C NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif
VOID Unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS RegisterCallback(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS UnRegisterCallback();

// The registry and transaction callback routines
EX_CALLBACK_FUNCTION Callback;
NTSTATUS Callback(IN PVOID CallbackContext, IN PVOID Argument1, IN PVOID Argument2);

VOID DeleteValue(PVOID context, PREG_DELETE_VALUE_KEY_INFORMATION info);
VOID SetValue(PVOID context, PREG_SET_VALUE_KEY_INFORMATION info);
VOID CreateKey(PVOID context, PREG_CREATE_KEY_INFORMATION info);
VOID OpenKey(PVOID context, PREG_OPEN_KEY_INFORMATION info);

BOOLEAN GetRegistryObjectCompleteName(PUNICODE_STRING pRegistryPath, PUNICODE_STRING pPartialRegistryPath, PVOID pRegistryObject);