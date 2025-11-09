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
#include <ctime>
#include <csignal>

using namespace std;

// Structure to hold process info
struct ProcessInfo {
    int pid;
    string name;
    double cpuUsage;
    double memUsage;
};

// Function to read total memory
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

// Function to get all running processes
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

        string name;
        string skip;
        unsigned long utime = 0, stime = 0;

        statFile >> skip >> name;
        for (int i = 0; i < 11; i++) statFile >> skip;
        statFile >> utime >> stime;

        double cpuUsage = (utime + stime) / 100.0;

        // Memory usage
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
    // Start ncurses mode
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE); // non-blocking key input
    start_color();

    // Define color pairs
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    bool running = true;

    while (running) {
        

        vector<ProcessInfo> procs = getProcesses();

        // Sort by CPU usage first, then by Memory usage
        sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            if (a.cpuUsage == b.cpuUsage)
                return a.memUsage > b.memUsage;
            return a.cpuUsage > b.cpuUsage;
        });

        // Title and header
        mvprintw(0, 0, "===== Simple System Monitor (Day 5 - Real-Time + Kill Feature) =====");
        mvprintw(1, 0, "Press 'q' to quit | Press 'k' to kill a process | Auto-refresh every 2s");
        mvprintw(3, 0, "PID     %-20s  CPU%%   MEM%%", "Process Name");
        mvhline(4, 0, '-', 50);

        // Display top 10 processes
        int row = 5;
        for (int i = 0; i < (int)procs.size() && i < 10; ++i) {
            const auto& p = procs[i];

            if (p.cpuUsage > 10.0)
                attron(COLOR_PAIR(3));
            else if (p.cpuUsage > 5.0)
                attron(COLOR_PAIR(2));
            else
                attron(COLOR_PAIR(1));

            mvprintw(row++, 0, "%-7d %-20s %-7.2f %-7.2f",
                     p.pid, p.name.substr(0, 18).c_str(),
                     p.cpuUsage, p.memUsage);

            attroff(COLOR_PAIR(1));
            attroff(COLOR_PAIR(2));
            attroff(COLOR_PAIR(3));
        }

        // Show timestamp of last update
        time_t now = time(0);
        string timeStr = ctime(&now);
        timeStr.pop_back(); // remove newline
        mvprintw(17, 0, "Last Updated: %s", timeStr.c_str());
        mvprintw(18, 0, "(Top 10 processes sorted by CPU and Memory)");

        // Check for user input
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = false;
        } 
        else if (ch == 'k' || ch == 'K') {
            echo();
            nodelay(stdscr, FALSE);
            mvprintw(20, 0, "Enter PID to kill: ");
            int pid;
            scanw("%d", &pid);

            if (kill(pid, SIGKILL) == 0)
                mvprintw(21, 0, "Process %d killed successfully.", pid);
            else
                mvprintw(21, 0, "Failed to kill %d. (Try using sudo)", pid);

            noecho();
            nodelay(stdscr, TRUE);
        }

        refresh();
        this_thread::sleep_for(chrono::seconds(2)); // refresh every 2 seconds
    }

    endwin();
    return 0;
}

