#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace std;

// Structure to hold process information
struct ProcessInfo {
    int pid;
    string name;
    double cpuUsage;
    double memUsage;
};

// Function to read total memory from /proc/meminfo
long getTotalMemory() {
    ifstream memFile("/proc/meminfo");
    string key;
    long memTotal = 0;
    while (memFile >> key) {
        if (key == "MemTotal:") {
            memFile >> memTotal;
            break;
        }
        memFile.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return memTotal;
}

// Function to collect all running processes and their CPU & memory usage
vector<ProcessInfo> getProcesses() {
    vector<ProcessInfo> processes;
    DIR* dir = opendir("/proc");
    if (!dir) return processes;

    struct dirent* entry;
    long totalMem = getTotalMemory();

    while ((entry = readdir(dir)) != nullptr) {
        if (!isdigit(entry->d_name[0])) continue;  // Only numeric folders are process IDs

        int pid = stoi(entry->d_name);
        string statPath = "/proc/" + string(entry->d_name) + "/stat";
        ifstream statFile(statPath);
        if (!statFile) continue;

        string name, skip;
        unsigned long utime = 0, stime = 0;
        statFile >> skip >> name;
        for (int i = 0; i < 11; i++) statFile >> skip; // skip unwanted data
        statFile >> utime >> stime;

        double cpuUsage = (utime + stime) / 100.0;

        // Read memory info
        string statusPath = "/proc/" + string(entry->d_name) + "/status";
        ifstream statusFile(statusPath);
        string key;
        long memKB = 0;
        while (statusFile >> key) {
            if (key == "VmRSS:") {
                statusFile >> memKB;
                break;
            }
            statusFile.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        double memUsage = (double)memKB / totalMem * 100.0;
        processes.push_back({pid, name, cpuUsage, memUsage});
    }
    closedir(dir);
    return processes;
}

int main() {
    cout << "===== System Monitor (Day 2–3) =====" << endl;
    cout << "Displays running processes with CPU & Memory usage" << endl;
    cout << "Sorted by CPU usage (desc), then by Memory usage (desc)" << endl;
    cout << "Press Ctrl + C to stop the monitor" << endl;
    this_thread::sleep_for(chrono::seconds(2));

    while (true) {
        system("clear");
        vector<ProcessInfo> procs = getProcesses();

        // Sort first by CPU, then by Memory
        sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            if (a.cpuUsage == b.cpuUsage)
                return a.memUsage > b.memUsage;
            return a.cpuUsage > b.cpuUsage;
        });

        cout << left << setw(8) << "PID"
             << setw(20) << "Process"
             << setw(10) << "CPU%"
             << setw(10) << "MEM%" << endl;
        cout << string(50, '-') << endl;

        int count = 0;
        for (auto &p : procs) {
            if (count++ >= 10) break;
            cout << left << setw(8) << p.pid
                 << setw(20) << p.name.substr(0, 18)
                 << setw(10) << fixed << setprecision(2) << p.cpuUsage
                 << setw(10) << fixed << setprecision(2) << p.memUsage
                 << endl;
        }

        cout << "\n(Top 10 processes — Refreshing every 2 seconds...)" << endl;
        this_thread::sleep_for(chrono::seconds(2));
    }
    return 0;
}

