#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <dirent.h>
#include <sys/ioctl.h>

void panic(const char* msg){
    // escape sequences
    std::cerr << msg << std::endl;
    exit(1);
}

struct meminfo{
    int total = 0;
    int free = 0;
    int available = 0;
    void loadmem();
    int used(){return total-free;}
    int taken(){return total-available;}
    int percent(){if (total == 0){return 0;} else return 100*taken()/total;}
};
void meminfo::loadmem() {
    FILE* file = fopen("/proc/meminfo", "r");
    int line = 0, n = 0;
    char c[84];
    fread(&c, 1, 84, file);
    fclose(file);
    for (int i = 0; i < 84; i++) {
        if (c[i] > 47 && c[i] < 58) ((n *= 10) += c[i] - 48);
        else if (c[i] == 10) {
            n /= 1000;
            switch (line) {
                case 0: total = n;
                case 1: free = n;
                case 2: available = n;
            }
            n = 0;
            line++;
        }
    }
}

struct cpuinfo{
    int temp = 0;
    int loadavg = 0;
    void loadcpu();
    int getloadavg();
    int gettemp();
};
int cpuinfo::getloadavg() {
    long double a[4], b[4];
    FILE *fp;
    char dump[50];
    fp = fopen("/proc/stat","r");
    fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3]);
    fclose(fp);
    usleep(1000000);
    fp = fopen("/proc/stat","r");
    fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&b[0],&b[1],&b[2],&b[3]);
    fclose(fp);
    return 100 * ((b[0]+b[1]+b[2]) - (a[0]+a[1]+a[2])) / ((b[0]+b[1]+b[2]+b[3]) - (a[0]+a[1]+a[2]+a[3]));
}

int cpuinfo::gettemp() {
    struct dirent *item;
    FILE* file;
    char buf[12];

    DIR *dir = opendir("/sys/class/thermal");
    while ((item = readdir(dir))) {
        if (strncmp(item->d_name, "thermal_zone", 12) != 0) continue;

        char path[256] = "/sys/class/thermal/";
        strcat(path, item->d_name);
        strcat(path, "/type");

        file = fopen(path, "r");
        fread(&buf, 1, 12, file);
        fclose(file);

        if (strncmp(buf, "x86_pkg_temp", 12) == 0) {
            path[strlen(path) - 4] = '\0';
            strcat(path, "temp");

            file = fopen(path, "r");
            fread(&buf, 1, 2, file);
            fclose(file);
            return ((int)buf[0]-48)*10+((int)buf[1]-48);
        }
    }
    return 0;
}
void cpuinfo::loadcpu(){
    loadavg = getloadavg();
    temp = gettemp();
}

void termsize(int& w, int& h){
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    w = size.ws_col;
    h = size.ws_row;
}

void signal_callback_handler(int signum) {
    std::cout << "\033[?1049l\033[?25h" << std::endl;
    exit(signum);
}

int main(){
    std::cout << "\033[?25l\033[?1049h\033[2J" << std::endl;
    signal(SIGINT, signal_callback_handler);
    meminfo mem;
    cpuinfo cpu;
    int cpugraph[150] = {0};
    int ramgraph[150] = {0};
    for (;;) {
        int w, h;
        termsize(w, h);
        mem.loadmem();
        cpu.loadcpu();
        for (int i = 150; i > 0; i--) {
            cpugraph[i] = cpugraph[i-1];
            ramgraph[i] = ramgraph[i-1];
        }
        cpugraph[0] = cpu.loadavg;
        ramgraph[0] = mem.percent();

        std::cout << "\033[2J" << std::endl;
        std::cout << "\033[2;4HLoad: " << cpu.loadavg << "%\033[3;4HTemp: " << cpu.temp << " C" << std::endl;
        std::cout << "\033[2;22HUsed by apps: " << mem.taken() << "  (" << mem.percent() << "%)\033[2;55HIn use: " << mem.used() << std::endl;
        std::cout << "\033[3;22HCan be freed: " << mem.available << "\033[3;50HUnallocated: " << mem.free << std::endl;
        for (int i = 3; i < w/2-3; i++) if (cpugraph[w/2-i-3] > 0) std::cout << "\033[" << h-1-((cpugraph[w/2-i-3])*(h-7)/100) << ";" << i << "H*" << std::endl;
        for (int i = w/2+3; i < w-3; i++) /*if (ramgraph[i-w/2-3] > 0)*/ std::cout << "\033[" << h-1-((ramgraph[i-w/2-3])*(h-7)/100) << ";" << i << "H*" << std::endl;
    }
    return 0;
}