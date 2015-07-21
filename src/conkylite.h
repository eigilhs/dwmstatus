#ifndef CL_CPU_COUNT
#define CL_CPU_COUNT 2
#endif

struct cpu {
  unsigned long long idle;
  unsigned long long nonidle;
  unsigned int prct;
};

struct info {
  struct wireless_info *winfo;
  struct cpu cpu[CL_CPU_COUNT+1];
  unsigned int mem_total;
  unsigned int mem_avail;
  unsigned int temp[CL_TEMP_COUNT];
  char time[128];
  char ba_capacity[4];
  char ba_status;
};

struct conky_monitor {
  struct info *info;
  struct udev *udev;
  struct udev_monitor *mon;
  pthread_t thread;
};

static void get_time(struct info *);
static void get_cpu_usage(struct info *);
static void get_temp(struct info *);
static void get_mem(struct info *);
static void get_wireless(struct info *);
static void get_battery_capacity(struct info *);
static void get_battery_status(struct info *);
static void catch_sigint(int);
static void battery_cleanup(void *);
static void *battery_status_monitor_thread(void *);
static int start_monitors(struct info *, struct conky_monitor **);
static void stop_monitors(struct conky_monitor *, int);
static void update(struct info *);
static void info_create(struct info *);
static void info_free(struct info *);
static void set_root_name(struct info *, XTextProperty);
