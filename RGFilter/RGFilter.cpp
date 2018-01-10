
#include "RGFilter.h"

PDEVICE_OBJECT g_DeviceObj;

ULONG g_MajorVersion;
ULONG g_MinorVersion;

LARGE_INTEGER g_Cookie;

//////////////////////////////////////////////////////////////////////////////////
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	KdPrint(("[%s:%d] \n", _FN_, _LN_));

	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING NtDeviceName;
	UNICODE_STRING DosDevicesLinkName;

	RtlInitUnicodeString(&NtDeviceName, NT_DEVICE_NAME);
	RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICES_LINK_NAME);

	ntStatus = IoCreateDevice(DriverObject,
		0,
		&NtDeviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&g_DeviceObj);

	if (!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	ntStatus = IoCreateSymbolicLink(&DosDevicesLinkName, &NtDeviceName);

	if (!NT_SUCCESS(ntStatus))
	{
		IoDeleteDevice(DriverObject->DeviceObject);
		return ntStatus;
	}


	// ----------------------------------------------------------------------

	CmGetCallbackVersion(&g_MajorVersion, &g_MinorVersion);
	KdPrint(("Callback version %u.%u \n", g_MajorVersion, g_MinorVersion));

	// CmRegisterCallbackEx call.
	RegisterCallback(DriverObject->DeviceObject);

	// ----------------------------------------------------------------------


	DriverObject->DriverUnload = Unload;
	return ntStatus;
}

VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("[%s:%d] \n", _FN_, _LN_));

	UNICODE_STRING DosDevicesLinkName;
	RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICES_LINK_NAME);
	IoDeleteSymbolicLink(&DosDevicesLinkName);

	IoDeleteDevice(DriverObject->DeviceObject);

	// CmUnRegisterCallbackEx call.
	UnRegisterCallback();
}

//////////////////////////////////////////////////////////////////////////////////
NTSTATUS RegisterCallback(IN PDEVICE_OBJECT DeviceObject)
{
	KdPrint(("[%s:%d] \n", _FN_, _LN_));

	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING usAltitude;
	RtlInitUnicodeString(&usAltitude, L"360010"); //322132

	// Register the callback
	ntStatus = CmRegisterCallbackEx(Callback,
		&usAltitude,
		DeviceObject->DriverObject,
		NULL,
		&g_Cookie,
		NULL);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("CmRegisterCallbackEx failed. ntStatus => 0x%x \n", ntStatus));
	}

	return ntStatus;
}

NTSTATUS Callback(IN PVOID CallbackContext, IN PVOID Argument1, IN PVOID Argument2)
{
	REG_NOTIFY_CLASS NotifyClass;
	NotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

	if (Argument2 == NULL)
	{
		KdPrint(("[%s:%d] Callback Argument2 is NULL. \n", _FN_, _LN_));
		return STATUS_SUCCESS;
	}

	switch (NotifyClass)
	{
	case RegNtDeleteValueKey:
	{
		DeleteValue(CallbackContext, (PREG_DELETE_VALUE_KEY_INFORMATION)Argument2);
		break;
	}
	case RegNtSetValueKey:
	{
		SetValue(CallbackContext, (PREG_SET_VALUE_KEY_INFORMATION)Argument2);
		break;
	}
	case RegNtPreQueryValueKey:
	{
		PREG_QUERY_VALUE_KEY_INFORMATION info = (PREG_QUERY_VALUE_KEY_INFORMATION)Argument2;
		KdPrint(("[RGFilter!QueryValue] Value: %wZ\n", info->ValueName));
		break;
	}
	case RegNtPreCreateKeyEx:
	{
		CreateKey(CallbackContext, (PREG_CREATE_KEY_INFORMATION)Argument2);
		break;
	}
	case RegNtPreOpenKeyEx:
	{
		OpenKey(CallbackContext, (PREG_OPEN_KEY_INFORMATION)Argument2);
		break;
	}
	case RegNtPostDeleteValueKey:
	case RegNtPostSetValueKey:
	case RegNtPostQueryValueKey:
	case RegNtPostCreateKeyEx:
	case RegNtPostOpenKeyEx:
	{
		PREG_POST_OPERATION_INFORMATION info = (PREG_POST_OPERATION_INFORMATION)Argument2;
		KdPrint(("[%s]\n", (NT_SUCCESS(info->Status) ? "Success" : "Failure")));
		break;
	}
	default:
		//KdPrint(("[%s:%d] NotifyClass => %s \n", _FN_, NotifyClass));
		break;
	}

	return STATUS_SUCCESS;
}

NTSTATUS UnRegisterCallback()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	// Unregister the callback with the cookie
	ntStatus = CmUnRegisterCallback(g_Cookie);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("CmUnRegisterCallbackEx failed. ntStatus => 0x%x \n", ntStatus));
	}

	return ntStatus;
}

//////////////////////////////////////////////////////////////////////////////////
VOID DeleteValue(PVOID context, PREG_DELETE_VALUE_KEY_INFORMATION info)
{
	UNREFERENCED_PARAMETER(context);
	KdPrint(("[RGFilter!%s] Value: %wZ\n", _FN_, info->ValueName));
}

VOID SetValue(PVOID context, PREG_SET_VALUE_KEY_INFORMATION info)
{
	UNREFERENCED_PARAMETER(context);

	switch (info->Type)
	{
	case 0ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_NONE, Data: NULL\n", _FN_, info->ValueName));
		break;
	}
	case 1ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_SZ, Data: %ws\n", _FN_, info->ValueName, (WCHAR*)info->Data));
		break;
	}
	case 2ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_EXPAND_SZ, Data: %ws\n", _FN_, info->ValueName, (WCHAR*)info->Data));
		break;
	}
	case 3ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_BINARY, Data: BINARY...SKIP\n", _FN_, info->ValueName));
		break;
	}
	case 4ul:
	case 5ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_DWORD, Data: 0x%08x (%d)\n", _FN_, info->ValueName, *(DWORD*)info->Data, *(DWORD*)info->Data));
		break;
	}
	case 6ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_LINK, Data: %ws\n", _FN_, info->ValueName, (WCHAR*)info->Data));
		break;
	}
	case 7ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_MULTI_SZ", _FN_, info->ValueName));

		/*WCHAR* data = (WCHAR*)info->Data;
		WCHAR* pos = (WCHAR*)info->Data;
		BOOLEAN check = FALSE;
		int i = 0;
		
		if (data[0] == '\0') {
			KdPrint((" {%d : NULL}\n", ++i));
			break;
		}
		while (true) {
			if (check && data[0] != '\0') {
				KdPrint((","));
			}
			check = FALSE;
			if (pos[0] == '\0') {
				KdPrint((" {%d : %ws}", ++i, data));
				data = pos + 1;
				check = TRUE;
			}
			if (data[0] == '\0') {
				KdPrint(("\n"));
				break;
			}
			pos++;
		}*/
		break;
	}
	case 8ul:
	case 9ul:
	case 10ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_RESOURCE, Data: BINARY...SKIP\n", _FN_, info->ValueName)); 
		break;
	}
	case 11ul:
	{
		KdPrint(("[RGFilter!%s] Value: %wZ, Type: REG_QWORD, Data: 0x%016x (%d)\n", _FN_, info->ValueName, info->Type, *(QWORD*)info->Data), *(QWORD*)info->Data);
		break;
	}
	default:
	{
		break;
	}
	}
}

VOID CreateKey(PVOID context, PREG_CREATE_KEY_INFORMATION  info)
{
	UNREFERENCED_PARAMETER(context);
	UNICODE_STRING registryPath;
	BOOLEAN RootCheck;

	if (info->CompleteName->Buffer != NULL)
	{
		registryPath.Length = 0;
		registryPath.MaximumLength = NTSTRSAFE_UNICODE_STRING_MAX_CCH * sizeof(WCHAR);
		registryPath.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, registryPath.MaximumLength, REGISTRY_POOL_TAG);
		RootCheck = GetRegistryObjectCompleteName(&registryPath, info->CompleteName, info->RootObject);
		if (RootCheck)
		{
			RtlUnicodeStringCatString(&registryPath, L"\\");
			RtlUnicodeStringCat(&registryPath, info->CompleteName);
		}
		KdPrint(("[RGFilter!%s] RegistryPath: %wZ\n", _FN_, &registryPath));
		ExFreePoolWithTag(registryPath.Buffer, REGISTRY_POOL_TAG);
	}
}

VOID OpenKey(PVOID context, PREG_OPEN_KEY_INFORMATION info) 
{
	UNREFERENCED_PARAMETER(context);
	UNICODE_STRING registryPath;
	BOOLEAN RootCheck;

	if (info->CompleteName->Buffer != NULL)
	{
		registryPath.Length = 0;
		registryPath.MaximumLength = NTSTRSAFE_UNICODE_STRING_MAX_CCH * sizeof(WCHAR);
		registryPath.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, registryPath.MaximumLength, REGISTRY_POOL_TAG);
		RootCheck = GetRegistryObjectCompleteName(&registryPath, info->CompleteName, info->RootObject);
		if (RootCheck)
		{
			RtlUnicodeStringCatString(&registryPath, L"\\");
			RtlUnicodeStringCat(&registryPath, info->CompleteName);
		}
		KdPrint(("[RGFilter!%s] RegistryPath: %wZ\n", _FN_, &registryPath));
		ExFreePoolWithTag(registryPath.Buffer, REGISTRY_POOL_TAG);
	}
}

//////////////////////////////////////////////////////////////////////////////////
BOOLEAN GetRegistryObjectCompleteName(PUNICODE_STRING pRegistryPath, PUNICODE_STRING pPartialRegistryPath, PVOID pRegistryObject)
{
	UNREFERENCED_PARAMETER(pPartialRegistryPath);

	if (pPartialRegistryPath->Buffer[0] == '\\'){
		{
			RtlUnicodeStringCopy(pRegistryPath, pPartialRegistryPath);
			return FALSE;
		}
	}

	NTSTATUS status;
	ULONG returnedLength;
	PUNICODE_STRING	pObjectName = NULL;

	status = ObQueryNameString(pRegistryObject, (POBJECT_NAME_INFORMATION)pObjectName, 0, &returnedLength);
	if (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		pObjectName = (PUNICODE_STRING)ExAllocatePoolWithTag(NonPagedPool, returnedLength, REGISTRY_POOL_TAG);
		status = ObQueryNameString(pRegistryObject, (POBJECT_NAME_INFORMATION)pObjectName, returnedLength, &returnedLength);
		if (NT_SUCCESS(status))
		{
			RtlUnicodeStringCopy(pRegistryPath, pObjectName);
		}
		ExFreePoolWithTag(pObjectName, REGISTRY_POOL_TAG);
	}
	return TRUE;
}