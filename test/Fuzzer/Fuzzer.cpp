#include <windows.h>
#include <stdio.h>
#include <time.h>

#define FILE_DEVICE_LEYLINE 0x22
#define LEYLINE_IOCTL_BASE 0x800

#define IOCTL_LEYLINE_GET_STATUS CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LEYLINE_MAP_BUFFER CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LEYLINE_MAP_PARAMS CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LEYLINE_CREATE_CABLE CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

ULONG ioctls[] = { IOCTL_LEYLINE_GET_STATUS, IOCTL_LEYLINE_MAP_BUFFER, IOCTL_LEYLINE_MAP_PARAMS, IOCTL_LEYLINE_CREATE_CABLE };

int main()
{
    printf("Starting Fuzzer for Leyline Audio Driver CDO...\n");
    HANDLE hDevice = CreateFileW(L"\\\\.\\Global\\LeylineAudio", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        hDevice = CreateFileW(L"\\\\.\\LeylineAudio", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice == INVALID_HANDLE_VALUE) {
            hDevice = CreateFileW(L"\\\\.\\Global\\LeylineAudio", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hDevice == INVALID_HANDLE_VALUE) {
                 hDevice = CreateFileW(L"\\\\.\\LeylineAudio", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
                 if (hDevice == INVALID_HANDLE_VALUE) {
                     printf("Failed to open device. Error code: %lu\n", GetLastError());
                     return 1;
                 }
            }
        }
    }

    srand((unsigned)time(NULL));
    BYTE buffer[1024];

    for (int i = 0; i < 10000; i++) {
        ULONG ioctl = ioctls[rand() % 4];
        DWORD bytesReturned;
        for (int j = 0; j < 1024; j++) buffer[j] = rand() % 256;
        
        DeviceIoControl(hDevice, ioctl, buffer, rand() % 1024, buffer, rand() % 1024, &bytesReturned, NULL);
        
        if (i % 1000 == 0) printf("Iter %d completed.\n", i);
    }
    
    CloseHandle(hDevice);
    printf("Fuzzing complete.\n");
    return 0;
}
