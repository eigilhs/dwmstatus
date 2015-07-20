#define STAT "/proc/stat"
#define LOAD "/proc/loadavg"
#define MEM  "/proc/meminfo"
#define TEMP "/sys/class/hwmon/"
#define BATT "/sys/class/power_supply/BAT0/"
#define AC   "/sys/class/power_supply/AC/"

#define WIFACE "wlan0"

struct info {
  struct wireless_info *winfo;
  struct cpu *cpu;
  int *mem;
  int *temp;
  char *time;
  char *ba_capacity;
  char ba_status;
};

struct cpu {
  unsigned long long idle[4];
  unsigned long long nonidle[4];
  unsigned int prct[4];
};

struct conky_monitor {
  struct info *info;
  pthread_t thread;
  struct udev_monitor *mon;
  struct udev *udev;
};

static void get_time(struct info *);
static void get_cpu_usage(struct info *);
static void get_temp(struct info *);
static void get_mem(struct info *);
static void get_wireless(struct info *);
static void get_battery_capacity(struct info *);
static void get_battery_status(struct info *);
static void print(struct info *);
static void catch_sigint(int);
static void battery_cleanup(void *);
static void *battery_status_monitor_thread(void *);
static int start_monitors(struct info *, struct conky_monitor **);
static void stop_monitors(struct conky_monitor *, int);
static void update(struct info *);
static void info_malloc(struct info *);
static void info_free(struct info *);
