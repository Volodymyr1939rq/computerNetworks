#include <windows.h>
#include <stdio.h>
#include <string.h>

struct ThreadParams {
    int threadId;
    int start;
    int end;
    int step;
    HANDLE hSemaphore;
    HANDLE hEvent;
    CRITICAL_SECTION* pCS;
    int syncType; 
};
struct SharedData {
    int offset;             
    char text[1024 * 1024]; 
};

HANDLE hMapFile;
SharedData*pData;


DWORD WINAPI ThreadFunc(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;

  
    WaitForSingleObject(params->hSemaphore, INFINITE);

    for (int i = params->start; (params->step > 0 ? i <= params->end : i >= params->end); i += params->step) {
        
        if (params->syncType == 1) WaitForSingleObject(params->hEvent, INFINITE);
        else if (params->syncType == 2) EnterCriticalSection(params->pCS);

        
      char tempBuf[32];
        sprintf_s(tempBuf, "[Thread %d] %d\n", params->threadId, i); 
        
        printf("%s", tempBuf); 
        
        int len = strlen(tempBuf);
      
        if (pData->offset + len < sizeof(pData->text)) {
            memcpy(pData->text + pData->offset, tempBuf, len);
            pData->offset += len; 
        }

        if (params->syncType == 1) SetEvent(params->hEvent);
        else if (params->syncType == 2) LeaveCriticalSection(params->pCS);
    }

    ReleaseSemaphore(params->hSemaphore, 1, NULL);
    return 0;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    HANDLE hFile = CreateFileA("output_numbers.txt", 
                               GENERIC_READ | GENERIC_WRITE, 
                               0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    SetFilePointer(hFile, sizeof(SharedData), NULL, FILE_BEGIN);
    SetEndOfFile(hFile);

    hMapFile = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, sizeof(SharedData), "Local\\MyTextMapping");
    pData = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    
    pData->offset = 0;
    
    HANDLE hSem = CreateSemaphore(NULL, 2, 2, NULL); 
    HANDLE hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);

    HANDLE hThreads[6];
    ThreadParams params[6];

    for (int i = 0; i < 6; i++) {
        params[i].threadId = i + 1;
        params[i].start = (i % 2 == 0) ? 1 : -1;
        params[i].end = (i % 2 == 0) ? 500 : -500;
        params[i].step = (i % 2 == 0) ? 1 : -1;
        params[i].hSemaphore = hSem;
        params[i].hEvent = hEvent;
        params[i].pCS = &cs;
        params[i].syncType = i / 2;

        
        hThreads[i] = CreateThread(NULL, 0, ThreadFunc, &params[i], CREATE_SUSPENDED, NULL);

        // SetThreadPriority(hThreads[i], (i % 2 == 0) ? THREAD_PRIORITY_HIGHEST : THREAD_PRIORITY_LOWEST);
    }

    printf("Starting threads...\n");
    for (int i = 0; i < 6; i++) ResumeThread(hThreads[i]); 
    Sleep(50);

    WaitForMultipleObjects(6, hThreads, TRUE, INFINITE);

    printf("\nAll threads finished work.\n");
    DeleteCriticalSection(&cs);
    CloseHandle(hSem);
    CloseHandle(hEvent);
    UnmapViewOfFile(pData);
    CloseHandle(hMapFile);
    CloseHandle(hFile);

    system("pause");
    return 0;
}