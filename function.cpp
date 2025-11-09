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
#include <ncurses.h>
#include <csignal>

using namespace std;

// Structure for process info
struct ProcessInfo {
    int pid;
    string name;
    double cpuUsage;
    double memUsage;
};

// Function to get total system memory
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

// Function to read processes
vector<ProcessInfo> getProcesses() {
    vector<ProcessInfo> processes;
    DIR* dir = opendir("/proc");
    if (!dir) return processes;

    struct dirent* entry;
    long totalMem = getTotalMemory();

    while ((entry = readdir(dir)) != nullptr) {
        if (!isdigit(entry->d_name[0])) continue;
        int pid = stoi(entry->d_name);
        string statPath = "/proc/" + string(entry->d_name) + "/stat";
        ifstream statFile(statPath);
        if (!statFile) continue;

        string name, skip;
        unsigned long utime = 0, stime = 0;
        statFile >> skip >> name;
        for (int i = 0; i < 11; i++) statFile >> skip;
        statFile >> utime >> stime;

        double cpuUsage = (utime + stime) / 100.0;

        // Memory info
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

// Function to draw UI
void drawUI(const vector<ProcessInfo>& procs) {
    clear();
    mvprintw(0, 0, "===== System Monitor (Day 4) =====");
    mvprintw(1, 0, "Press 'q' to quit | Press 'k' to kill process");
    mvprintw(3, 0, "PID     %-20s  CPU%%   MEM%%", "Process Name");
    mvhline(4, 0, '-', 50);

    int row = 5;
    for (int i = 0; i < (int)procs.size() && i < 10; ++i) {
        const auto& p = procs[i];

        // Color based on CPU usage
        if (p.cpuUsage > 10.0)
            attron(COLOR_PAIR(3)); // red
        else if (p.cpuUsage > 5.0)
            attron(COLOR_PAIR(2)); // yellow
        else
            attron(COLOR_PAIR(1)); // green

        mvprintw(row++, 0, "%-7d %-20s %-7.2f %-7.2f",
                 p.pid, p.name.substr(0, 18).c_str(),
                 p.cpuUsage, p.memUsage);

        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));
        attroff(COLOR_PAIR(3));
    }
    refresh();
}

int main() {
    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    bool running = true;

    while (running) {
        vector<ProcessInfo> procs = getProcesses();

        // Sort by CPU, then by Memory
        sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            if (a.cpuUsage == b.cpuUsage)
                return a.memUsage > b.memUsage;
            return a.cpuUsage > b.cpuUsage;
        });

        drawUI(procs);

        int ch = getch();
        if (ch == 'q') running = false;
        else if (ch == 'k') {
            echo();
            nodelay(stdscr, FALSE);
            mvprintw(17, 0, "Enter PID to kill: ");
            int pid;
            scanw("%d", &pid);
            if (kill(pid, SIGKILL) == 0)
                mvprintw(18, 0, "Process %d killed successfully.", pid);
            else
                mvprintw(18, 0, "Failed to kill %d ", pid);
            noecho();
            nodelay(stdscr, TRUE);
        }

        refresh();
        this_thread::sleep_for(chrono::seconds(2));
    }

    endwin();
    return 0;
}

