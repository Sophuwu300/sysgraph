#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <dirent.h>

void panic(const char* msg){
    // escape sequences
    std::cerr << msg << std::endl;
    exit(1);
}

struct meminfo{
    int total;
    int free;
    int available;
    void loadinfo();
    int used(){return total-free;}
    int taken(){return total-available;}
};
void meminfo::loadinfo() {
    FILE* file = fopen("/proc/meminfo", "r");
    int line, n = 0;
    char c[84];
    fread(c, 1, 84, file);
    fclose(file);
    for (int i = 0; i < 84; i++) {
        if (c[i] > 47 && c[i] < 58)(n *= 10) += c[i] - 48;
        else if (c[i] == 10) {
            n /= 1000;
            switch (line) {
                case 0:
                    total = n;
                case 1:
                    free = n;
                case 2:
                    available = n;
                    break;
            }
            n = 0;
            line++;
        }
    }
}

struct cpuinfo{
    int temp;
    int loadavg;
    void loadinfo();
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
}
void cpuinfo::loadinfo(){
    loadavg = getloadavg();
    temp = gettemp();
}


int main(){
    cpuinfo cpu;
    cpu.loadinfo();
    printf("CPU load: %d\n", cpu.loadavg);
    printf("CPU temp: %d\n", cpu.temp);
    meminfo mem;
    mem.loadinfo();
    printf("Memory used: %d\n", mem.used());
    printf("Memory taken: %d\n", mem.taken());
    printf("Memory free: %d\n", mem.free);
    printf("Memory available: %d\n", mem.available);
    printf("Memory total: %d\n", mem.total);
    return 0;
}