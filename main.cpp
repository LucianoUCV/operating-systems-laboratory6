#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cmath>

using namespace std;

bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i <= sqrt(n); i++) {
        if (n % i == 0) return false;
    }
    return true;
}

void find_primes_in_range(int start, int end, int write_fd) {
    vector<int> primes;
    for (int i = start; i <= end; i++) {
        if (is_prime(i)) {
            primes.push_back(i);
        }
    }
    write(write_fd, primes.data(), primes.size() * sizeof(int));
    close(write_fd);
}

int main() {
    const int NUM_PROCESSES = 10;
    const int RANGE = 10000;
    const int CHUNK = RANGE / NUM_PROCESSES;

    int pipes[NUM_PROCESSES][2];

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pipe(pipes[i]);
        pid_t pid = fork();

        if (pid == 0) {
            close(pipes[i][0]);
            int start = i * CHUNK + 1;
            int end = (i + 1) * CHUNK;
            find_primes_in_range(start, end, pipes[i][1]);
            return 0;
        } else if (pid < 0) {
            cerr << "Failed to create process " << i << endl;
            return 1;
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        close(pipes[i][1]);
        int buffer[CHUNK] = {0};
        int num_bytes = read(pipes[i][0], buffer, sizeof(buffer));
        int num_primes = num_bytes / sizeof(int);
        for (int j = 0; j < num_primes; j++) {
            cout << buffer[j] << " ";
        }
        close(pipes[i][0]);
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(nullptr);
    }

    return 0;
}
