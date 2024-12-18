#include <iostream>
#include <windows.h>
#include <vector>
#include <cmath>
#include <string>

using namespace std;

bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i <= sqrt(n); i++) {
        if (n % i == 0) return false;
    }
    return true;
}

void find_primes_in_range(int start, int end, HANDLE write_pipe) {
    vector<int> primes;
    for (int i = start; i <= end; i++) {
        if (is_prime(i)) {
            primes.push_back(i);
        }
    }
    DWORD bytes_written;
    WriteFile(write_pipe, primes.data(), primes.size() * sizeof(int), &bytes_written, nullptr);
    CloseHandle(write_pipe);
}

int main(int argc, char* argv[]) {
    const int NUM_PROCESSES = 10;
    const int RANGE = 10000;
    const int CHUNK = RANGE / NUM_PROCESSES;

    if (argc == 3) {
        int start = stoi(argv[1]);
        int end = stoi(argv[2]);

        HANDLE write_pipe = GetStdHandle(STD_OUTPUT_HANDLE);
        if (write_pipe == INVALID_HANDLE_VALUE) {
            cerr << "Invalid pipe handle for child process.\n";
            return 1;
        }

        find_primes_in_range(start, end, write_pipe);
        return 0;
    }

    HANDLE pipes[NUM_PROCESSES][2];
    PROCESS_INFORMATION process_info[NUM_PROCESSES] = {};
    STARTUPINFO startup_info[NUM_PROCESSES] = {};

    for (int i = 0; i < NUM_PROCESSES; i++) {
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        if (!CreatePipe(&pipes[i][0], &pipes[i][1], &sa, 0)) {
            cerr << "Failed to create pipe " << i << endl;
            return 1;
        }

        startup_info[i] = { sizeof(STARTUPINFO) };
        startup_info[i].hStdOutput = pipes[i][1];
        startup_info[i].dwFlags |= STARTF_USESTDHANDLES;

        int start = i * CHUNK + 1;
        int end = (i + 1) * CHUNK;

        string command = string(argv[0]) + " " + to_string(start) + " " + to_string(end);
        vector<char> command_buffer(command.begin(), command.end());
        command_buffer.push_back('\0');

        if (!CreateProcess(nullptr, command_buffer.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup_info[i], &process_info[i])) {
            cerr << "Failed to create process " << i << " Error: " << GetLastError() << endl;
            return 1;
        }

        CloseHandle(pipes[i][1]); 
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        int buffer[CHUNK] = { 0 };
        DWORD bytes_read;
        ReadFile(pipes[i][0], buffer, sizeof(buffer), &bytes_read, nullptr);
        int num_primes = bytes_read / sizeof(int);
        for (int j = 0; j < num_primes; j++) {
            cout << buffer[j] << " ";
        }
        CloseHandle(pipes[i][0]);
        WaitForSingleObject(process_info[i].hProcess, INFINITE);
        CloseHandle(process_info[i].hProcess);
        CloseHandle(process_info[i].hThread);
    }

    return 0;
}
