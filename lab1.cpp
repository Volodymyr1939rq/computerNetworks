#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
   
     if (argc > 1) {
        int processNum = atoi(argv[1]);
        HANDLE hAnonMutex = (HANDLE)atoi(argv[2]);
        HANDLE hSem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "lab_1_semaphore");

        WaitForSingleObject(hSem, INFINITE);
        printf("Process #%d (PID: %d) is working...\n", processNum, GetCurrentProcessId());
        
        Sleep(2000); 

        ReleaseSemaphore(hSem, 1, NULL);
        CloseHandle(hSem);
        return 0;
    }
    
    HANDLE hSingleRun = CreateMutexA(NULL, FALSE, "lab_1_singlerun");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        printf("Process already running! Closing...\n");
        return 0;
    }

    printf("Main Process (PID: %d) started.\n", GetCurrentProcessId());

    HANDLE hSemaphore = CreateSemaphoreA(NULL, 3, 3, "lab_1_semaphore");
    
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hAnonMutex = CreateMutex(&sa, FALSE, NULL);

    PROCESS_INFORMATION pi[10];
    for (int i = 1; i <= 10; i++) {
        STARTUPINFOA si = { sizeof(si) };
        char cmdLine[255];
        sprintf_s(cmdLine, "%s %d %d", argv[0], i, (int)hAnonMutex);  

        CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi[i-1]);
    }

    HANDLE hTimer = CreateWaitableTimerA(NULL, TRUE, "lab_1_timer");
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -5000 * 10000LL; 
    SetWaitableTimer(hTimer, &dueTime, 0, NULL, NULL, FALSE);

    printf("Main process waiting for 5 seconds...\n");
    WaitForSingleObject(hTimer, INFINITE);

    for (int i = 0; i < 10; i++) {
        DWORD exitCode;
        GetExitCodeProcess(pi[i].hProcess, &exitCode);
        if (exitCode == STILL_ACTIVE) 
            printf("Process %d is still working.\n", i + 1);
        else 
            printf("Process %d finished.\n", i + 1);
        
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    CloseHandle(hSemaphore);
    CloseHandle(hAnonMutex);
    CloseHandle(hTimer);
    CloseHandle(hSingleRun);

    system("pause");
    return 0;
}