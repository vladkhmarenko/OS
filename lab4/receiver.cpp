#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

const int DELAY = 25000;
const int MAX_MESSAGE_LENGTH = 20;
const int EXIT_DELAY = 500;

int main() {

    STARTUPINFO si;
    PROCESS_INFORMATION pi{nullptr};
    std::string binFilename;
    int recordsNumber = 0;
    int processesNumber = 0;

    std::cout << "Enter .bin file name: ";
    std::cin >> binFilename;
    std::cout << "Enter number of records: ";
    std::cin >> recordsNumber;
    std::cout << "Enter number of processes: ";
    std::cin >> processesNumber;

    // Validate number of records
    if(recordsNumber < 0) {
        std::cerr << "Number of records can't be negative";
        return 1;
    }

    // Create synchronization objects
    HANDLE hCurrentProcess = CreateMutex(nullptr, TRUE, "Current Process Owner");
    HANDLE hReadAccessSemaphore = CreateSemaphore(nullptr, 0, recordsNumber, "Read Access Semaphore");
    HANDLE hWriteAccessSemaphore = CreateSemaphore(nullptr, recordsNumber, recordsNumber, "Write Access Semaphore");

    HANDLE *hProcesses = new HANDLE[processesNumber];
    HANDLE *hStartEvents = new HANDLE[processesNumber];

    // Start Sender processes
    for(int i = 0; i < processesNumber; i++) {
        char* num;
        std::cout << "Start process " << i << "\n";
        hStartEvents[i] = CreateEvent(nullptr, TRUE, FALSE, (LPCSTR) (num));
    }

    std::string request;
    for (int i = 0; i < processesNumber; i++) {
        request = std::string("Sender.exe") + " " + binFilename + " " + std::to_string(i);
        if(!CreateProcess(nullptr, &request[0], nullptr, nullptr, TRUE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
            std::cerr << "Failed to create process" << std::endl;
        }
        CloseHandle(pi.hThread);
        hProcesses[i] = pi.hProcess;
    }

    // Wait for all Sender processes to start
    WaitForMultipleObjects(processesNumber, hStartEvents, TRUE, INFINITE);
    ReleaseMutex(hCurrentProcess);

    int action = 0;
    DWORD response;
    char message[MAX_MESSAGE_LENGTH];

    // Process user actions
    while(action != 2) {
        WaitForSingleObject(hCurrentProcess, INFINITE);
        response = WaitForSingleObject(hReadAccessSemaphore, DELAY);
        if(response == WAIT_TIMEOUT) {
            std::cerr << "No response (timeout)";
            break;
        }

        std::cout << "Choose action: " << std::endl;
        std::cout << "1 - Read" << std::endl;
        std::cout << "2 - Exit" << std::endl;
        std::cin >> action;

        std::ifstream binInputStream;
        int bytesRead = 0;
        switch(action) {
            case 1:
                // Read message from the binary file
                binInputStream.open(binFilename, std::ios::binary);
                binInputStream.seekg(bytesRead);

                if(binInputStream.tellg() != 0 || !binInputStream.eof()) {
                    binInputStream.read((char *) &message, sizeof(message));
                    std::cout << "Message: ";
                    for(char i: message) std::cout << i;
                    std::cout << std::endl;
                    binInputStream.close();
                    bytesRead += MAX_MESSAGE_LENGTH;
                    ReleaseSemaphore(hWriteAccessSemaphore, 1, nullptr);
                }
                else {
                    continue;
                }
                break;
            case 2:
                std::cout << "Exit..." << std::endl;
                Sleep(EXIT_DELAY);
                break;
            default:
                std::cout << "Invalid action chosen" << std::endl;
                continue;
        }
    }

    CloseHandle(hCurrentProcess);
    CloseHandle(hReadAccessSemaphore);
    CloseHandle(hWriteAccessSemaphore);

    for (std::size_t i = 0; i < processesNumber; i++)
        CloseHandle(hStartEvents[i]);
    delete[] hStartEvents;

    for (std::size_t i = 0; i < processesNumber; i++)
        CloseHandle(hProcesses[i]);
    delete[] hProcesses;

    return 0;
}
