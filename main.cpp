#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include "tinydir.h"

void panic(const char* msg){
    // escape sequences
    std::cerr << msg << std::endl;
    exit(1);
}

struct meminfo{
    int total;
    int free;
    int available;
    void get();
    int used(){return total-free;}
    int taken(){return total-available;}
};
void meminfo::get(){
    FILE* file = fopen("/proc/meminfo", "r");
    if(file == NULL) panic("ERROR: Unable to open proc file");
    int line, n = 0;
    while(!feof(file)) {
        char c = fgetc(file);
        if (c > 47 && c < 58)(n*=10)+=c-48;
        else if (c == 10) {
            n/=1000;
            switch(line){
                case 0: total = n;
                case 1: free = n;
                case 2: available = n; break;
            }
            n = 0;
            line++;
        }
    }
    fclose(file);
}

struct cpuinfo{
    int temp;
    int loadavg;
    void get();
    int getloadavg();
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
int cpuinfo::gettemp(){

}
void cpuinfo::get(){
    loadavg = getloadavg();

}


int main(){
    cpuinfo cpu;
    cpu.get();
    printf("CPU load: %d\n", cpu.loadavg);
}