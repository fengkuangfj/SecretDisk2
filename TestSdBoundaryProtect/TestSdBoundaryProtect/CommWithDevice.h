/*

// TestWDMDriver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

#define DEVICE_NAME L"\\\\.\\HelloWDM"

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hDevice = CreateFile(DEVICE_NAME,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if (hDevice != INVALID_HANDLE_VALUE)
	{
		char* inbuf = "hello world";
		char outbuf[12] = {0};
		DWORD dwBytes = 0;
		BOOL b = DeviceIoControl(hDevice, CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_IN_DIRECT, FILE_ANY_ACCESS), 
			inbuf, 11, outbuf, 11, &dwBytes, NULL);

		for (int i = 0; i < 11; i++)
		{
			outbuf[i] = outbuf[i] ^ 'm';
		}

		printf("DeviceIoControl, ret: %d, outbuf: %s, outbuf address: %x operated: %d bytes\n", b, outbuf, outbuf, dwBytes);

		CloseHandle(hDevice);

	}
	else
		printf("CreateFile failed, err: %x\n", GetLastError());

	return 0;
}

#define IOCTL_ENCODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

NTSTATUS HelloWDMIOControl(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	KdPrint(("Enter HelloWDMIOControl\n"));

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	//得到输入缓冲区大小
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;

	//得到输出缓冲区大小
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;

	//得到IOCTRL码
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	NTSTATUS status;
	ULONG info = 0;
	switch (code)
	{
	case IOCTL_ENCODE:
		{
			//获取输入缓冲区，IRP_MJ_DEVICE_CONTROL的输入都是通过buffered io的方式
			char* inBuf = (char*)Irp->AssociatedIrp.SystemBuffer;
			for (ULONG i = 0; i < cbin; i++)//将输入缓冲区里面的每个字节和m亦或
			{
				inBuf[i] = inBuf[i] ^ 'm';
			}

			//获取输出缓冲区，这里使用了直接方式，见CTL_CODE的定义，使用了METHOD_IN_DIRECT。所以需要通过直接方式获取out buffer
			KdPrint(("user address: %x, this address should be same to user mode addess.\n", MmGetMdlVirtualAddress(Irp->MdlAddress)));
			//获取内核模式下的地址，这个地址一定> 0x7FFFFFFF,这个地址和上面的用户模式地址对应同一块物理内存
			char* outBuf = (char*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

			ASSERT(cbout >= cbin);
			RtlCopyMemory(outBuf, inBuf, cbin);
			info = cbin;
			status = STATUS_SUCCESS;
		}
		break;
	default:
		status = STATUS_INVALID_VARIANT;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloWDMIOControl\n"));
	return status;
}

*/