static char *DS_TEMP_SENSORS[DS_TEMP_COUNT] = {DS_SENSORS};

struct cpu {
  unsigned long long idle;
  unsigned long long nonidle;
  unsigned int prct;
  unsigned int __padding;
};

struct info {
  struct cpu cpu[DS_CPU_COUNT+1];
  unsigned long mem_total;
  unsigned long mem_avail;
  unsigned int temp[DS_TEMP_COUNT];
  int wi_bitrate;
  char wi_essid[IW_ESSID_MAX_SIZE + 1];
  char time[122];
  char ba_capacity[4];
  char ba_status;
};

struct ds_monitor {
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
static int start_monitors(struct info *, struct ds_monitor **);
static void stop_monitors(struct ds_monitor *, int);
static void update(struct info *);
static void info_create(struct info *);
static void set_root_name(struct info *);
static void extract_cpu_times(char *, int, ...);
