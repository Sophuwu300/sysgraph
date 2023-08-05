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

int w, h;

struct meminfo{
    int total = 0;
    int free = 0;
    int available = 0;
    void loadmem();
    int used(){return total-free;}
    int taken(){return total-available;}
    int percent(){if (total == 0){return 0;} else return 100*taken()/total;}
    int bufpercent(){if (total == 0){return 0;} else return 100*used()/total;}
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

std::string padint(int n, int len){
    std::string s = std::to_string(n);
    for (; s.length() < len; s = " " + s);
    return s;
}

int graphscale(int n) {return h-1-(n*(h-6)/100);}

void plot(int x, int y, int color){
    if (color > 7 || color < 1) color = 7;
    if (x%2 == 0) color+=8;
    for (;y<h;std::cout << "\033[" << y++ << ";" << x << "H\033[48;5;" << color << "m \033[0m");
}

int main(){
    meminfo mem;
    cpuinfo cpu;
    int cpugraph[150] = {0};
    int ramgraph[150] = {0};
    int i;
    std::string bg;
    std::string graphnum;
    std::cout << "\033[?25l\033[?1049h\033[2J" << std::endl;
    signal(SIGINT, signal_callback_handler);
    for (;;) {
        termsize(w, h);

        mem.loadmem();
        cpu.loadcpu(); // contains usleep(1000000)

        for (i = 150; i > 0; i--) {
            cpugraph[i] = cpugraph[i-1];
            ramgraph[i] = ramgraph[i-1];
        }
        cpugraph[0] = cpu.loadavg;
        ramgraph[0] = mem.percent();

        if (w < 60 || h < 10) {
            std::cout << "\033[2J\033[1;1HWindow too small\033[2;1HMinimum size: 60x10" << std::endl;
            continue;
        }
        if (w > 300) w = 300;
        if (h > 106) h = 106;

        std::cout
        << "\033[2J"
        << "\033[2;3HLoad: " << padint(cpu.loadavg, 3) << "% Memory:\033[3;3HTemp: " << padint(cpu.temp, 3) << "C"
        << "\033[2;22HUsed by apps: " << padint(mem.taken(), 5) << " (" << mem.percent() << "%)\033[2;48HUsed: " << padint(mem.used(), 5)
        << "\033[3;22HCan be freed: " << padint(mem.available, 5) << "\033[3;48HFree: " << padint(mem.free, 5);

        for (bg = ""; bg.length() < w-4; bg += " ");
        for (i = 5; i < h; i++) {
            std::cout << "\033[" << i << ";3H\033[48;5;235m" << bg << "\033[0m";
        }

        for (i = 3; i < w/2-2; i++) {
            plot(i, graphscale(cpugraph[w/2+1-i-3]), 4);
            plot(i+w/2+2, graphscale(ramgraph[w/2+1-i-3]), 6);
        }

        for (i = 5; i < h; i++) {
            graphnum = padint(100-(i-5)*100/(h-6), 3);
            std::cout << "\033[" << i << ";" << w/2-2 << "H -" << graphnum << "- ";
        }
        std::cout << std::endl;
    }
    return 0;
}