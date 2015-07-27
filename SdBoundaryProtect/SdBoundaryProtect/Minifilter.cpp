/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		Minifilter.cpp
*
* Abstract:
*
*		无
*
* Environment:
*
*		Kernel mode
*
* Version:
*
*		无
*
* Author:
*
*		岳翔
*
* Complete Time:
*
*		无
*
* Modify Record:
*
*		无
*
--*/

#include "stdafx.h"
#include "Minifilter.h"


CMinifilter * CMinifilter::ms_pMfIns = NULL;


BOOLEAN
	CMinifilter::InitCallbacks()
{
	BOOLEAN bRet	= FALSE;

	ULONG	ulIndex = 0;


	__try
	{
		RtlZeroMemory(m_Callbacks, sizeof(m_Callbacks));

		m_Callbacks[ulIndex].MajorFunction	= IRP_MJ_CREATE;
		m_Callbacks[ulIndex].Flags			= 0;
		m_Callbacks[ulIndex].PreOperation	= PreCreate;
		m_Callbacks[ulIndex].PostOperation	= NULL;

		ulIndex++;
		m_Callbacks[ulIndex].MajorFunction	= IRP_MJ_SET_INFORMATION;
		m_Callbacks[ulIndex].Flags			= 0;
		m_Callbacks[ulIndex].PreOperation	= PreSetInformation;
		m_Callbacks[ulIndex].PostOperation	= NULL;

		ulIndex++;
		m_Callbacks[ulIndex].MajorFunction	= IRP_MJ_DIRECTORY_CONTROL;
		m_Callbacks[ulIndex].Flags			= 0;
		m_Callbacks[ulIndex].PreOperation	= PreDirectoryControl;
		m_Callbacks[ulIndex].PostOperation	= PostDirectoryControl;

		ulIndex++;
		if (CALLBACKS_NUM - 1 != ulIndex)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ulIndex error. ulIndex(%d)",
				ulIndex);

			m_Callbacks[CALLBACKS_NUM - 1].MajorFunction = IRP_MJ_OPERATION_END;

			__leave;
		}

		m_Callbacks[ulIndex].MajorFunction = IRP_MJ_OPERATION_END;

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

VOID
	CMinifilter::InitRegistration()
{
	RtlZeroMemory(&m_FltRegistration, sizeof(m_FltRegistration));

	m_FltRegistration.Size								= sizeof(m_FltRegistration);
	m_FltRegistration.Version							= FLT_REGISTRATION_VERSION;
	m_FltRegistration.Flags								= 0;
	m_FltRegistration.ContextRegistration				= NULL;
	m_FltRegistration.OperationRegistration				= m_Callbacks;
	m_FltRegistration.FilterUnloadCallback				= Unload;
	m_FltRegistration.InstanceSetupCallback				= InstanceSetup;
	m_FltRegistration.InstanceQueryTeardownCallback		= InstanceQueryTeardown;
	m_FltRegistration.InstanceTeardownStartCallback		= InstanceTeardownStart;
	m_FltRegistration.InstanceTeardownCompleteCallback	= InstanceTeardownComplete;
	m_FltRegistration.GenerateFileNameCallback			= NULL;
	m_FltRegistration.NormalizeNameComponentCallback	= NULL;
	m_FltRegistration.NormalizeContextCleanupCallback	= NULL;

	return;
}

NTSTATUS
	DriverEntry(
	__in PDRIVER_OBJECT		DriverObject,
	__in PUNICODE_STRING	RegistryPath
	)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;


	UNREFERENCED_PARAMETER(RegistryPath);


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		CMinifilter::ms_pMfIns = new(MEMORY_TAG_MINIFILTER) CMinifilter;

		if (!CMinifilter::ms_pMfIns->Register(DriverObject))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pMfIns->Register failed");
			__leave;
		}

		ntStatus = STATUS_SUCCESS;
	}
	__finally
	{
		;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return ntStatus;
}

BOOLEAN
	CMinifilter::Register(
	__in PDRIVER_OBJECT pDriverObject
	)
{
	BOOLEAN		bRet		= FALSE;

	NTSTATUS	ntStatus	= STATUS_UNSUCCESSFUL;

	CLog		Log;
	CFileName	FileName;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		GetLock();

		if (!Log.Init())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Log.Init failed");
			__leave;
		}

		if (!FileName.Init())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.Init failed");
			__leave;
		}

		// Register with FltMgr to tell it our callback routines
		ntStatus = FltRegisterFilter(
			pDriverObject,
			&m_FltRegistration,
			&m_pFltFilter
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltRegisterFilter failed. (%x)",
				ntStatus);

			__leave;
		}

		// Start filtering i/o
		ntStatus = FltStartFiltering(m_pFltFilter);
		if (!NT_SUCCESS(ntStatus)) 
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltStartFiltering failed. (%x)",
				ntStatus);

			__leave;
		}

		m_AllModuleInit = TRUE;

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();

		if (!bRet)
		{
			Log.Unload();

			if (m_pFltFilter)
			{
				FltUnregisterFilter(m_pFltFilter);
				m_pFltFilter = NULL;
			}

			FileName.Unload();
		}
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

VOID
	CMinifilter::GetLock()
{
	KeEnterCriticalRegion();

	return;
}

VOID
	CMinifilter::FreeLock()
{
	KeLeaveCriticalRegion();

	return;
}

CMinifilter::CMinifilter()
{
	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	m_pFltFilter = NULL;
	m_AllModuleInit = FALSE;
	m_bUnloading = FALSE;
	m_bAllowFltWork = FALSE;

	InitCallbacks();
	InitRegistration();

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");
}

CMinifilter::~CMinifilter()
{
	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	m_pFltFilter = NULL;
	m_AllModuleInit = FALSE;
	m_bUnloading = FALSE;
	m_bAllowFltWork = FALSE;

	RtlZeroMemory(&m_Callbacks, sizeof(m_Callbacks));
	RtlZeroMemory(&m_FltRegistration, sizeof(m_FltRegistration));

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");
}

NTSTATUS
	CMinifilter::Unload(
	__in FLT_FILTER_UNLOAD_FLAGS Flags
	)
{
	NTSTATUS	ntStatus = STATUS_UNSUCCESSFUL;

	CLog		Log;
	CFileName	FileName;


	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		CMinifilter::ms_pMfIns->GetLock();

		CMinifilter::ms_pMfIns->m_bUnloading = TRUE;

		CMinifilter::ms_pMfIns->m_AllModuleInit = FALSE;

		CMinifilter::ms_pMfIns->m_bAllowFltWork = FALSE;

		Log.Unload();

		if (CMinifilter::ms_pMfIns->m_pFltFilter)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"FltUnregisterFilter begin");

			FltUnregisterFilter(CMinifilter::ms_pMfIns->m_pFltFilter);

			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"FltUnregisterFilter end");

			CMinifilter::ms_pMfIns->m_pFltFilter = NULL;
		}

		FileName.Unload();
	}
	__finally
	{
		CMinifilter::ms_pMfIns->FreeLock();

		delete CMinifilter::ms_pMfIns;
		CMinifilter::ms_pMfIns = NULL;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return STATUS_SUCCESS;
}

NTSTATUS
	CMinifilter::InstanceSetup(
	__in PCFLT_RELATED_OBJECTS		FltObjects,
	__in FLT_INSTANCE_SETUP_FLAGS	Flags,
	__in DEVICE_TYPE				VolumeDeviceType,
	__in FLT_FILESYSTEM_TYPE		VolumeFilesystemType
	)
{
	NTSTATUS				ntStatus											= STATUS_UNSUCCESSFUL;

	UCHAR					volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES) + 512]  = {0};
	PFLT_VOLUME_PROPERTIES	pVolProp											= (PFLT_VOLUME_PROPERTIES)volPropBuffer;
	ULONG					retLen												= 0;
	BOOLEAN					bRemoveable											= FALSE;
	PDEVICE_OBJECT			pDevObj												= NULL;
	PUNICODE_STRING			pWorkingName										= NULL;
	USHORT					size												= 0;
	USHORT					usCutOffset											= 0;
	UNICODE_STRING			ustrDosName											= {0};
	BOOLEAN					bModify												= FALSE;
	ULONG					ulSectorSize										= 0;
	BOOLEAN					bOnlyDevName										= FALSE;

	CKrnlStr				VolDevName;
	CKrnlStr				VolAppName;
	CKrnlStr				VolSymName;
	CKrnlStr				OldVolDevName;

	CFileName				FileName;


	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);

	PAGED_CODE();


	__try 
	{
		if (!CMinifilter::ms_pMfIns->CheckEnv(MINIFILTER_ENV_TYPE_RUNING))
			__leave;

		// Always get the volume properties, so I can get a sector size
		ntStatus = FltGetVolumeProperties( 
			FltObjects->Volume,
			pVolProp,
			sizeof(volPropBuffer),
			&retLen 
			);
		if (!NT_SUCCESS(ntStatus)) 
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltAllocateContext failed. (%x)",
				ntStatus);

			__leave;
		}

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"(%p) (%d) (%x) (%x) %wZ %wZ %wZ",
			FltObjects->Instance,
			VolumeFilesystemType,
			pVolProp->DeviceType,
			pVolProp->DeviceCharacteristics,
			&pVolProp->FileSystemDriverName,
			&pVolProp->FileSystemDeviceName,
			&pVolProp->RealDeviceName);

		if (pVolProp->DeviceCharacteristics)
			bRemoveable = TRUE;

		// Save the sector size in the context for later use.  Note that
		// we will pick a minimum sector size if a sector size is not
		// specified.
		if (pVolProp->SectorSize && MIN_SECTOR_SIZE > pVolProp->SectorSize)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"SectorSize error. SectorSize(%d)",
				pVolProp->SectorSize);

			__leave;
		}

		ulSectorSize = max(pVolProp->SectorSize, MIN_SECTOR_SIZE);

		// Get the storage device object we want a name for.
		ntStatus = FltGetDiskDeviceObject(FltObjects->Volume, &pDevObj);
		if (NT_SUCCESS(ntStatus)) 
		{
			// Try and get the DOS name.  If it succeeds we will have
			// an allocated name buffer.  If not, it will be NULL
			ntStatus = RtlVolumeDeviceToDosName(pDevObj, &ustrDosName);
			if (!NT_SUCCESS(ntStatus))
				KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"RtlVolumeDeviceToDosName failed. (%x)",
				ntStatus);
			else
				KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"DosName(%wZ)",
				&ustrDosName);
		}
		else
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"FltGetDiskDeviceObject failed. (%x)",
			ntStatus);

		//  If we could not get a DOS name, get the NT name.
		if (!NT_SUCCESS(ntStatus)) 
		{
			// Figure out which name to use from the properties
			if (0 < pVolProp->RealDeviceName.Length) 
			{
				pWorkingName = &pVolProp->RealDeviceName;

				KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"RealDeviceName(%wZ)",
					pWorkingName);
			}
			else if (0 < pVolProp->FileSystemDeviceName.Length) 
			{
				pWorkingName = &pVolProp->FileSystemDeviceName;

				KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"FileSystemDeviceName(%wZ)",
					pWorkingName);
			}
			else 
			{
				// No name, don't save the context
				KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"no name");
				__leave;
			}

			// Get size of buffer to allocate.  This is the length of the
			// string plus room for a trailing colon.
			size = pWorkingName->Length + sizeof(WCHAR);

			// Now allocate a buffer to hold this name
			ustrDosName.Buffer = (PWCHAR)new(MEMORY_TAG_INSTANCE_TAG) CHAR[size];

			// Init the rest of the fields
			ustrDosName.Length = 0;
			ustrDosName.MaximumLength = size;

			// Copy the name in
			RtlCopyUnicodeString(&ustrDosName, pWorkingName);

			// Put a trailing colon to make the display look good
			RtlAppendUnicodeToString(&ustrDosName, L":");
		}

		// 开始保存卷名信息

		// 获得卷的设备名
		if (!CFileName::GetVolDevNameFromFltVol(FltObjects->Volume, &VolDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CFileName::GetDevNameFromFltObj failed");
			__leave;
		}

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"DevName(%wZ)",
			VolDevName.Get());

		if (!VolDevName.ToUpper())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolDevName.ToUpper failed. DevName(%wZ)",
				VolDevName.Get());

			__leave;
		}

		// 获得卷的R3名
		if (L'\\' == *ustrDosName.Buffer)
		{
			if (L'\\' == *(ustrDosName.Buffer + 1) && L'?' == *(ustrDosName.Buffer + 2))
			{
				// pVolCtx->Name中为\\?\volume{}
				if (!FileName.GetVolAppNameByQueryObj(&VolDevName, &VolAppName, &usCutOffset))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"GetVolAppNameByQueryObj failed. DevName(%wZ)",
						VolDevName.Get());

					__leave;
				}
			}
			else
				bOnlyDevName = TRUE;
		}
		else
		{
			// pVolCtx->Name中为a-z格式
			if (!VolAppName.Set(&ustrDosName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolAppName.Set failed. DosName(%wZ)",
					&ustrDosName);

				__leave;
			}
		}

		if (!bOnlyDevName)
		{
			if (!VolAppName.ToUpper())
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolAppName.ToUpper failed. AppName(%wZ)",
					VolAppName.Get());

				__leave;
			}

			// 获得卷符号链接名
			if (!VolSymName.Set(L"\\??\\", wcslen(L"\\??\\")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolSymName.Set failed");
				__leave;
			}

			if (!VolSymName.Append(&VolAppName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolSymName.Append failed. AppName(%wZ)",
					VolAppName.Get());

				__leave;
			}
		}

		// 保存卷名信息
		if (!FileName.InsertVolNameInfo(&VolAppName, &VolSymName, &VolDevName, bOnlyDevName, bRemoveable, FltObjects->Instance, ulSectorSize, &bModify, &OldVolDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.InsertVolName failed. AppName(%wZ) SymName(%wZ) DevName(%wZ)",
				VolAppName.Get(), VolSymName.Get(), VolDevName.Get());

			__leave;
		}

		if (CMinifilter::ms_pMfIns->CheckEnv(MINIFILTER_ENV_TYPE_ALL_MODULE_INIT | MINIFILTER_ENV_TYPE_ALLOW_WORK))
		{
			if (bModify)
			{
				// 已经格式化完毕

			}
		}
	} 
	__finally 
	{
		// Remove the reference added to the device object by
		// FltGetDiskDeviceObject.
		if (pDevObj) 
		{
			ObDereferenceObject(pDevObj);
			pDevObj = NULL;
		}

		if (ustrDosName.Buffer)
		{
			delete ustrDosName.Buffer;
			ustrDosName.Buffer = NULL;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS
	CMinifilter::InstanceQueryTeardown(
	__in PCFLT_RELATED_OBJECTS				FltObjects,
	__in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS	Flags
	)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();


	return STATUS_SUCCESS;
}

VOID
	CMinifilter::InstanceTeardownStart(
	__in PCFLT_RELATED_OBJECTS			FltObjects,
	__in FLT_INSTANCE_TEARDOWN_FLAGS	Flags
	)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();


	return;
}

VOID
	CMinifilter::InstanceTeardownComplete(
	__in PCFLT_RELATED_OBJECTS			FltObjects,
	__in FLT_INSTANCE_TEARDOWN_FLAGS	Flags
	)
{
	CKrnlStr	VolDevName;

	CFileName	FileName;
	CLog		Log;


	PAGED_CODE();


	__try
	{
		if (!CFileName::GetVolDevNameFromFltVol(FltObjects->Volume, &VolDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CFileName::GetVolDevNameFromFltVol failed");
			__leave;
		}

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"(%p) (%x) (%wZ)",
			FltObjects->Instance, Flags, VolDevName.Get());

		if (FileName.IsDisMountStandard(&VolDevName, FltObjects->Instance))
		{
			if (!FileName.DelVolNameInfo(&VolDevName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.DelVolNameInfo failed. File(%wZ)",
					VolDevName.Get());

				__leave;
			}

			Log.GetLock();
			if (Log.ms_pFltInstance && Log.ms_pFltInstance == FltObjects->Instance)
			{
				Log.ms_pFltInstance = NULL;
				Log.ms_ulSectorSize = 0;
			}
			Log.FreeLock();
		}
	}
	__finally
	{
		;
	}

	return;
}

FLT_PREOP_CALLBACK_STATUS
	CMinifilter::PreCreate(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__deref_out_opt PVOID					*	CompletionContext
	)
{
	NTSTATUS status;


	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );


	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS
	CMinifilter::PreSetInformation(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__deref_out_opt PVOID					*	CompletionContext
	)
{
	NTSTATUS status;


	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );


	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS
	CMinifilter::PreDirectoryControl(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__deref_out_opt PVOID					*	CompletionContext
	)
{
	NTSTATUS status;


	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );


	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
	CMinifilter::PostDirectoryControl(
	__inout		PFLT_CALLBACK_DATA			Data,
	__in		PCFLT_RELATED_OBJECTS		FltObjects,
	__in_opt	PVOID						CompletionContext,
	__in		FLT_POST_OPERATION_FLAGS	Flags
	)
{
	UNREFERENCED_PARAMETER( Data );
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( CompletionContext );
	UNREFERENCED_PARAMETER( Flags );


	return FLT_POSTOP_FINISHED_PROCESSING;
}

BOOLEAN
	CMinifilter::CheckEnv(
	__in ULONG ulMinifilterEnvType
	)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		GetLock();

		if (MINIFILTER_ENV_TYPE_NULL == ulMinifilterEnvType)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"MinifilterEnvType error");
			__leave;
		}

		if (FlagOn(ulMinifilterEnvType, MINIFILTER_ENV_TYPE_RUNING))
		{
			if (m_bUnloading)
				__leave;
		}

		if (FlagOn(ulMinifilterEnvType, MINIFILTER_ENV_TYPE_ALL_MODULE_INIT))
		{
			if (!m_AllModuleInit)
				__leave;
		}

		if (FlagOn(ulMinifilterEnvType, MINIFILTER_ENV_TYPE_ALLOW_WORK))
		{
			if (!m_bAllowFltWork)
				__leave;
		}

		if (FlagOn(ulMinifilterEnvType, MINIFILTER_ENV_TYPE_FLT_FILTER))
		{
			if (!m_pFltFilter)
				__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}
