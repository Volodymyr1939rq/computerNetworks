#include <iostream>
#include <windows.h>
#include <string>

void RunChild(HANDLE hReadPipe) {
    std::cout << "[Child] Process started. Waiting for 3 distinct commands from Parent..." << std::endl;
    
    char buffer[1024];
    DWORD bytesRead;

    for (int i = 1; i <= 3; ++i) {
        std::cout << "[Child] Attempting read #" << i << "..." << std::endl;
        
        if (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            buffer[bytesRead] = '\0'; 
            std::cout << "[Child] SUCCESS on read #" << i << ". Received: " << buffer << "\n" << std::endl;
        } else {
            std::cout << "[Child] Read failed (Pipe broken)." << std::endl;
            break;
        }
    }
    
    std::cout << "[Child] All 3 commands received successfully!" << std::endl;
}

void RunParent() {
    std::cout << "[Parent] Starting application. Demonstrating 'Communication Difficulties'..." << std::endl;

    HANDLE hReadPipe, hWritePipe;
    
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; 
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        std::cerr << "CreatePipe failed!" << std::endl;
        return;
    }

    SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[256];
    sprintf(cmdLine, "lab5.exe child %p", hReadPipe);

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "CreateProcess failed!" << std::endl;
        return;
    }

    CloseHandle(hReadPipe);

    Sleep(500);

    std::cout << "[Parent] Sending 3 distinct commands (CMD_START, CMD_WORK, CMD_STOP)..." << std::endl;
    DWORD bytesWritten;
    
    const char* msg1 = "CMD_START";
    const char* msg2 = "CMD_WORK";
    const char* msg3 = "CMD_STOP";

    WriteFile(hWritePipe, msg1, strlen(msg1), &bytesWritten, NULL);
    WriteFile(hWritePipe, msg2, strlen(msg2), &bytesWritten, NULL);
    WriteFile(hWritePipe, msg3, strlen(msg3), &bytesWritten, NULL);

    std::cout << "[Parent] All messages sent! Waiting for child to process them..." << std::endl;

    DWORD waitResult = WaitForSingleObject(pi.hProcess, 5000);

    if (waitResult == WAIT_TIMEOUT) {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << " [ERROR] INDEFINITE POSTPONEMENT DETECTED!" << std::endl;
        std::cout << " The Child process is hanging on ReadFile." << std::endl;
        std::cout << " Reason: Pipe is a byte stream. The Child read all 3" << std::endl;
        std::cout << " messages in the FIRST read, and is now waiting forever" << std::endl;
        std::cout << " for the 2nd and 3rd messages that will never come." << std::endl;
        std::cout << "=======================================================\n" << std::endl;
        
        TerminateProcess(pi.hProcess, 1);
    } else {
        std::cout << "[Parent] Child finished successfull." << std::endl;
    }

    CloseHandle(hWritePipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main(int argc, char* argv[]) {
   
    if (argc == 3 && std::string(argv[1]) == "child") {
        HANDLE hRead;
        sscanf(argv[2], "%p", &hRead);
        RunChild(hRead);
    } 

    else {
        RunParent();
    }
    return 0;
}