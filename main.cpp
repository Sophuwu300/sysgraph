#include <string>
#include <csignal>
#include <sys/ioctl.h>
#include <chrono>

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

struct cpufile{
    std::string filepath;
    std::string filename;
    std::string comp;
    std::string usepath(int i) {
        std::string s = filepath;
        s.append(std::to_string(i));
        s.append(filename);
        return s;
    }
};

int findcpu(cpufile& ct)  {
    FILE* file;
    char buf[ct.comp.length()+1];
    int i = 0;
    if (ct.filename == "_label") i = 1;
    for (;;i++) {
        file = fopen(ct.usepath(i).c_str(), "r");
        if (file == NULL) return -1;
        fread(&buf, 1, ct.comp.length(), file);
        fclose(file);
        if ( std::string(buf) == ct.comp ) {
            return i;
        }
    }
}

int cpuinfo::gettemp() {
    FILE* file;
    char buf[2];
    cpufile ct = {"/sys/class/thermal/thermal_zone", "/temp", "x86_pkg_temp"};
    int fileNo = findcpu(ct);
    if (fileNo == -1) {
        ct = {"/sys/class/hwmon/hwmon", "/name", "k10temp"};
        fileNo = findcpu(ct);
        if (fileNo==-1) return 0;
        ct.filename = "_label";
        ct.filepath.append(std::to_string(fileNo)).append("/temp");
        ct.comp = "Tdie";
        fileNo = findcpu(ct);
        if (fileNo==-1) return 0;
        ct.filename = "_input";
    } else {
        ct.filename = "/temp";
    }
    file = fopen(ct.usepath(fileNo).c_str(), "r");
    if (file == NULL) return 0;
    fread(&buf, 1, 2, file);
    fclose(file);
    return (buf[0]-48)*10 + buf[1] - 48;
}

void cpuinfo::loadcpu(){
    loadavg = getloadavg();
    temp = gettemp();
}

void termsize(){
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    w = size.ws_col;
    h = size.ws_row;
}

void signal_callback_handler(int _) {
    printf("\033[?1049l\033[?25h");
    exit(0);
}

std::string padint(int n, int len){
    std::string s = std::to_string(n);
    for (; s.length() < len; s = " " + s);
    return s;
}

int graphscale(int n) {return h-1-(n*(h-6)/100);}

void plot(int x, int y, int color, std::string* s){
    if (color > 7 || color < 1) color = 7;
    if (x%2 == 0) color+=8;
    std::string print = std::string(";").append(std::to_string(x)).append("H\033[48;5;").append(std::to_string(color)).append("m \033[0m");
    for (;y<h;y++) s->append("\033[").append(std::to_string(y)).append(print);
}

int helpmenu(int argc, char* argv[]) {
    if (argc < 2) return 0;
    if (!(std::string(argv[1])=="-h" || std::string(argv[1])=="--help")) return 0;
    printf("\nUsage: %s [OPTION]\n\n", argv[0]);
    printf("  -h, --help\t\tDisplay this help message.\n");
    printf("      --log <file>\tLog to file (appends). Log frequency: 1 second. Format:\n");
    printf("            \t\tunix, cpu usage, temp, ram taken, buffer, avialbale, free\n\n");
    return 1;
}

int logarg(int argc, char* argv[], std::string& logdir) {
    if (argc < 3) return 0;
    if (std::string(argv[1]) != "--log") return 0;
    logdir = std::string(argv[2]);
    return 1;
}

void log(std::string logdir, meminfo mem, cpuinfo cpu) {
    std::string logbuf;
    logbuf.clear();
    logbuf.append(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()))
    .append(",").append(std::to_string(cpu.loadavg))
    .append(",").append(std::to_string(cpu.temp))
    .append(",").append(std::to_string(mem.taken()))
    .append(",").append(std::to_string(mem.used()-mem.taken()))
    .append(",").append(std::to_string(mem.available))
    .append(",").append(std::to_string(mem.free))
    .append("\n");
    FILE* f = fopen(logdir.c_str(), "a");
    fputs(logbuf.c_str(), f);
    fclose(f);
}

int main(int argc, char* argv[]){
    if (helpmenu(argc, argv)) return 0;
    std::string logdir;
    int dolog = logarg(argc, argv, logdir);
    meminfo mem;
    cpuinfo cpu;
    int cpugraph[150] = {0};
    int ramgraph[150] = {0};
    int bufgraph[150] = {0};
    int i;
    std::string bg;
    std::string graphnum;
    std::string tmp;
    std::string outbuf;
    outbuf.reserve(1000000);
    printf("\033[?25l\033[?1049h\033[2J");
    signal(SIGINT, signal_callback_handler);
    for (;;) {
        mem.loadmem();
        cpu.loadcpu(); // contains usleep(1000000)
        termsize();
        for (i = 150; i > 0; i--) {
            cpugraph[i] = cpugraph[i-1];
            ramgraph[i] = ramgraph[i-1];
            bufgraph[i] = bufgraph[i-1];
        }
        cpugraph[0] = cpu.loadavg;
        ramgraph[0] = mem.percent();
        bufgraph[0] = mem.bufpercent();

        if (dolog) log(logdir, mem, cpu);


        if (w < 60 || h < 10) {
            printf("\033[2J\033[1;1HWindow too small\033[2;1HMinimum size: 60x10\n");
            continue;
        }
        if (w > 300) w = 300;
        if (h > 106) h = 106;

        outbuf.append(std::string("\033[2J")
        .append("\033[2;3HLoad: ").append(padint(cpu.loadavg, 3))
        .append("% Memory:\033[3;3HTemp: ").append(padint(cpu.temp, 3)).append("C")
        .append("\033[2;22HUsed by apps: ").append(padint(mem.taken(), 5))
        .append(" (" + std::to_string(mem.percent())).append("%)\033[2;48HUsed: ")
        .append(padint(mem.used(), 5)).append("\033[3;22HCan be freed: ").append(padint(mem.available, 5))
        .append("\033[3;48HFree: ").append(padint(mem.free, 5)));

        for (bg = ";3H\033[48;5;235m"; bg.length() < w-4+14; bg.append(" "));
        bg.append("\033[0m");
        for (i = 5; i < h; i++) {
            outbuf.append("\033[").append(std::to_string(i)).append(bg);
        }
        outbuf.append("\0");

        for (i = 3; i < w/2-2+w%2; i++) {
            plot(i, graphscale(cpugraph[w/2+1-i-3]), 2, &outbuf);
            plot(i+w/2+1, graphscale(bufgraph[w/2+1-i-3]), 3, &outbuf);
            plot(i+w/2+1, graphscale(ramgraph[w/2+1-i-3]), 1, &outbuf);
        }
        outbuf.append("\0");

        tmp = ";" + std::to_string(w/2-2) + "H -";
        for (i = 5; i < h; i++) {
            outbuf.append("\033[")
            .append(std::to_string(i))
            .append(tmp)
            .append(padint(100-(i-5)*100/(h-6), 3)).append( + "- ");
        }
        printf("%s\n", outbuf.c_str());
        outbuf.clear();
    }
    return 0;
}