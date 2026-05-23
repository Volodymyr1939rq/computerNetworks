#include <iostream>
#include <windows.h>
#include <vector>

int N; 

std::vector<std::vector<double>> A;
std::vector<double> x; 

HANDLE hMutex;               
HANDLE hReadPipe, hWritePipe; 

struct Task {
    int k; 
    int i; 
    HANDLE hFinishEvent; 
};

DWORD WINAPI GaussWorker(LPVOID lpParam) {
    Task task;
    DWORD bytesRead;

    WaitForSingleObject(hMutex, INFINITE);
    ReadFile(hReadPipe, &task, sizeof(Task), &bytesRead, NULL);
    ReleaseMutex(hMutex);

    int k = task.k;
    int i = task.i;
    double factor = A[i][k] / A[k][k];

    for (int j = k; j <= N; j++) {
        A[i][j] -= factor * A[k][j];
    }

    WaitForSingleObject(hMutex, INFINITE);
    std::cout << "[Worker " << GetCurrentThreadId() << "] Finished row " << i << std::endl;
    ReleaseMutex(hMutex);

    SetEvent(task.hFinishEvent); 

    return 0; 
}

int main() {

    std::cout << "Enter the dimension of the matrix (N): ";
    std::cin >> N;

    A.resize(N, std::vector<double>(N + 1));
    x.resize(N);

    std::cout << "Enter the augmented matrix elements row by row (" << N << "x" << N + 1 << "):\n";
    for (int i = 0; i < N; i++) {
        for (int j = 0; j <= N; j++) {
            std::cin >> A[i][j];
        }
    }

    hMutex = CreateMutex(NULL, FALSE, NULL);
    CreatePipe(&hReadPipe, &hWritePipe, NULL, 0);

    std::cout << "\nStarting multi-threaded Gauss elimination (Finish-Finish sync)..." << std::endl;

    for (int k = 0; k < N - 1; k++) {
        int tasksCount = N - 1 - k;
        std::vector<HANDLE> finishEvents(tasksCount);
        std::vector<HANDLE> threads(tasksCount);

        for (int i = k + 1, taskIdx = 0; i < N; i++, taskIdx++) {
            finishEvents[taskIdx] = CreateEvent(NULL, TRUE, FALSE, NULL);
            Task t = {k, i, finishEvents[taskIdx]};
            DWORD bytesWritten;

            WriteFile(hWritePipe, &t, sizeof(Task), &bytesWritten, NULL);
            threads[taskIdx] = CreateThread(NULL, 0, GaussWorker, NULL, 0, NULL);
        }

        WaitForMultipleObjects(tasksCount, finishEvents.data(), TRUE, INFINITE);

        for (int idx = 0; idx < tasksCount; idx++) {
            CloseHandle(finishEvents[idx]);
            CloseHandle(threads[idx]);
        }
    }

    for (int i = N - 1; i >= 0; i--) {
        x[i] = A[i][N];
        for (int j = i + 1; j < N; j++) {
            x[i] -= A[i][j] * x[j];
        }
        x[i] /= A[i][i];
    }

    std::cout << "\nResults:" << std::endl;
    for (int i = 0; i < N; i++) {
        std::cout << "x[" << i + 1 << "] = " << x[i] << std::endl;
    }

    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    CloseHandle(hMutex);

    return 0;
}