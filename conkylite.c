#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <iwlib.h>
#include <libudev.h>

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
  char *load_avg;
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
static void get_load_avg(struct info *);
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

static volatile sig_atomic_t sig_status;

static void info_malloc(struct info *s)
{
  s->winfo = malloc(sizeof(struct wireless_info));
  s->mem = malloc(sizeof(int) * 2);
  s->load_avg = malloc(sizeof(char) * 15);
  s->temp = malloc(sizeof(int) * 3);
  s->time = malloc(sizeof(char) * 18);
  s->ba_capacity = malloc(sizeof(char) * 4);
  s->ba_status = 'U';
  s->cpu = malloc(sizeof(struct cpu));
  memset(s->cpu, 0, sizeof(struct cpu));
}

static void info_free(struct info *s)
{
  free(s->winfo);
  free(s->mem);
  free(s->load_avg);
  free(s->temp);
  free(s->time);
  free(s->ba_capacity);
  free(s->cpu);
}

static void get_load_avg(struct info *s)
{
  FILE *fp = fopen(LOAD, "r");
  if (fp) {
    fread(s->load_avg, 1, 14, fp);
    fclose(fp);
    s->load_avg[14] = 0;
  }
}

static void get_cpu_usage(struct info *s)
{
  unsigned long long usertime, nicetime, systemtime, idletime;
  unsigned long long ioWait, irq, softIrq, steal, guest, guestnice;
  unsigned long long systemalltime, idlealltime, totaltime, virtalltime;
  unsigned long long nonidlealltime, prevnonidle, previdle, prevtotaltime;
  int i, cpuid;
  char tmp[256];
  FILE *fp = fopen(STAT, "r");
  if (fp)
    fgets(tmp, 255, fp);
  for (i = 0; i < 4; i++) {
    ioWait = irq = softIrq = steal = guest = guestnice = 0;
    if (fp) {
      fgets(tmp, 255, fp);
      sscanf(tmp, "cpu%d %llu %llu %llu %llu %llu %llu %llu "
             "%llu %llu %llu", &cpuid, &usertime, &nicetime, &systemtime,
             &idletime, &ioWait, &irq, &softIrq, &steal, &guest, &guestnice);
    } else {
      fprintf(stderr, "Couldn't read %s\n", STAT);
      break;
    }
    systemalltime = systemtime + irq + softIrq;
    virtalltime = guest + guestnice;
    idlealltime = idletime + ioWait;
    nonidlealltime = usertime + nicetime + systemalltime + steal + virtalltime;
    totaltime = nonidlealltime + idlealltime;


    prevnonidle = s->cpu->nonidle[i];
    previdle = s->cpu->idle[i];
    prevtotaltime = prevnonidle + previdle;
    if (totaltime == prevtotaltime)
      break;
    s->cpu->prct[i] = (totaltime + previdle - (prevtotaltime + idlealltime))
      * 100 / (totaltime - prevtotaltime);

    s->cpu->nonidle[i] = nonidlealltime;
    s->cpu->idle[i] = idlealltime;
  }
  if (fp)
    fclose(fp);
}

static void get_temp(struct info *s)
{
  unsigned long t0 = 0, t1 = 0, t2 = 0;
  FILE *fp = fopen(TEMP "hwmon0/temp1_input", "r");
  if (fp) {
    fscanf(fp, "%lu", &t0);
    fclose(fp);
  }
  fp = fopen(TEMP "hwmon2/temp2_input", "r");
  if (fp) {
    fscanf(fp, "%lu", &t1);
    fclose(fp);
  }
  fp = fopen(TEMP "hwmon2/temp4_input", "r");
  if (fp) {
    fscanf(fp, "%lu", &t2);
    fclose(fp);
  }
  s->temp[0] = t0 * 0x418938 >> 32;
  s->temp[1] = t1 * 0x418938 >> 32;
  s->temp[2] = t2 * 0x418938 >> 32;
}

static void get_mem(struct info *s)
{
  FILE *fp = fopen(MEM, "r");
  if (fp) {
    fseek(fp, 14, 0);
    fscanf(fp, "%d", &s->mem[0]);
    fseek(fp, 46, 1);
    fscanf(fp, "%d", &s->mem[1]);
    fclose(fp);
  }
}

static void get_wireless(struct info *s)
{
  struct iwreq wrq;
  int skfd = iw_sockets_open();
  char iface[] = WIFACE;
  /* iw_get_basic_config(skfd, iface, &(s->winfo->b)); */
  /* iw_get_range_info(skfd, s, &(winfo->range)); */
  /* iw_get_stats(skfd, s, &(winfo->stats), &winfo->range, winfo->has_range); */
  /* iw_get_ext(skfd, s, SIOCGIWAP, &wrq); */
  /* memcpy(&(winfo->ap_addr), &(wrq.u.ap_addr), sizeof(sockaddr)); */
  wrq.u.essid.pointer = (caddr_t) s->winfo->b.essid;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  iw_get_ext(skfd, iface, SIOCGIWESSID, &wrq);
  if (iw_get_ext(skfd, iface, SIOCGIWRATE, &wrq) >= 0) {
    memcpy(&(s->winfo->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
  }
}

static void get_battery_capacity(struct info *s)
{
  FILE *fp = fopen(BATT "capacity", "r");
  if (fp) {
    fscanf(fp, "%s", s->ba_capacity);
    fclose(fp);
  }
}

static void get_battery_status(struct info *s)
{
  FILE *fp = fopen(BATT "status", "r");
  char c = 'U';
  if (fp) {
    s->ba_status = c = fgetc(fp);
    fclose(fp);
  }
  if (c == 'U') {
    fp = fopen(AC "online", "r");
    if (fp) {
      switch (fgetc(fp)) {
      case 49:                  /* '1' - on AC */
        c = 'A';
        break;
      case 48:                  /* '0' - on battery */
        c = 'D';
        break;
      }
      fclose(fp);
    }
  }
  s->ba_status = c;
}

static void print(struct info *s)
{
  /* printf("\r\x1b[K%u / %u / %u / %u %s :: %d / %d / %d C :: %.2f G :: %s %llu :: %c %s %% :: %s", */
  printf("%u / %u / %u / %u :: %d / %d / %d C :: %.2f G :: %s %llu :: %c %s %% :: %s\n",
         s->cpu->prct[0], s->cpu->prct[1], s->cpu->prct[2], s->cpu->prct[3],
         /* s->load_avg, */ s->temp[0], s->temp[1], s->temp[2],
         (s->mem[0] - s->mem[1]) / (float) 0x100000, s->winfo->b.essid,
         (unsigned long long) s->winfo->bitrate.value * 0x219 >> 32,
         s->ba_status, s->ba_capacity, s->time);
  fflush(stdout);
}

static void catch_sigint(int signo)
{
  sig_status = signo;
}

static void battery_cleanup(void *v)
{
  struct conky_monitor *m = (struct conky_monitor *) v;
  udev_monitor_unref(m->mon);
  udev_unref(m->udev);
}

static void *battery_status_monitor_thread(void *v)
{
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  struct conky_monitor *m = (struct conky_monitor *) v;
  struct info *s = m->info;
  int fd, ret;
  fd_set fds;
  static long TIMEOUT = 0x8000; /* Timeout ~ 9 minutes */
  
  m->udev = udev_new();
  m->mon = udev_monitor_new_from_netlink(m->udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(m->mon, "power_supply", NULL);
  udev_monitor_enable_receiving(m->mon);
  fd = udev_monitor_get_fd(m->mon);
  pthread_cleanup_push(battery_cleanup, m);

  get_battery_status(s);        /* Get status before blocking */

  for (;;) {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = {TIMEOUT, 0};

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    ret = select(fd+1, &fds, NULL, NULL, &tv);
    if (ret >= 0 && FD_ISSET(fd, &fds)) {
      usleep(500*1000);
      get_battery_status(s);
      if (s->ba_status == 'A') {
        sleep(1);               /* Wait for /sys to update */
        get_battery_status(s);
      }
    }
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  }
  pthread_cleanup_pop(NULL);
  return NULL;
}

static int start_monitors(struct info *s, struct conky_monitor **monitors)
{
  int NUMMONS = 1;
  *monitors = malloc(sizeof(struct conky_monitor) * NUMMONS);

  (*monitors)[0].info = s;
  if (pthread_create(&(monitors[0]->thread), NULL,
                     battery_status_monitor_thread,
                     (void *) monitors[0])) {
    fprintf(stderr, "Thread creation failed.\n");
    exit(EXIT_FAILURE);
  }
  return NUMMONS;
}

static void stop_monitors(struct conky_monitor *monitors, int nt)
{
  int i;
  for (i = 0; i < nt; i++) {
    pthread_cancel(monitors[i].thread);
    pthread_join(monitors[i].thread, NULL);
  }
  free(monitors);
}

static void get_time(struct info *s)
{
  time_t t;
  time(&t);
  strftime(s->time, 18, "%m-%V-%d %H:%M:%S", localtime(&t));
}

static void update(struct info *s)
{
  /* get_load_avg(s); */
  get_temp(s);
  get_mem(s);
  get_wireless(s);
  get_battery_capacity(s);
  get_cpu_usage(s);
  get_time(s);
  print(s);
}

int main(void)
{
  struct info s;
  struct conky_monitor *monitors;
  int nt;
  info_malloc(&s);
  signal(SIGINT, catch_sigint);
  update(&s);
  nt = start_monitors(&s, &monitors);
  while (!sig_status) {
    update(&s);
    sleep(1);
  }
  stop_monitors(monitors, nt);
  info_free(&s);
  puts("\r\x1b[KBye!");
  return 0;
}
