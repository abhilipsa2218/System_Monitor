#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

using namespace std;

double getCpuUsage() {
    static long prevIdle = 0, prevTotal = 0;
    ifstream file("/proc/stat");
    string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    long idleAll = idle + iowait;
    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long totalDiff = total - prevTotal;
    long idleDiff  = idleAll - prevIdle;
    prevTotal = total;
    prevIdle  = idleAll;
    if (totalDiff == 0) return 0.0;
    return (double)(totalDiff - idleDiff) / totalDiff * 100.0;
}

double getMemoryUsage() {
    ifstream file("/proc/meminfo");
    string key;
    long memTotal = 0, memAvailable = 0;
    while (file >> key) {
        if (key == "MemTotal:") file >> memTotal;
        else if (key == "MemAvailable:") { file >> memAvailable; break; }
        else file.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    if (memTotal == 0) return 0.0;
    return (double)(memTotal - memAvailable) / memTotal * 100.0;
}

int main() {
    cout << "===== Simple System Monitor (Day 1) =====" << endl;
    while (true) {
        double cpu = getCpuUsage();
        double mem = getMemoryUsage();
        cout << "\rCPU Usage: " << cpu << "%   |   Memory Usage: " << mem << "%" << flush;
        this_thread::sleep_for(chrono::seconds(1));
    }
    return 0;
}
