#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <stdarg.h>
#include <iwlib.h>
#include <libudev.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/sysinfo.h>

#include "config.h"
#include "dwmstatus.h"

static volatile sig_atomic_t sig_status;

static void catch_sigint(int signo)
{
  sig_status = signo;
}

int main(void)
{
  struct info s;
  struct conky_monitor *monitors;
  int nt;
  info_create(&s);
  signal(SIGINT, catch_sigint);

  update(&s);
  nt = start_monitors(&s, &monitors);
  while (!sig_status) {
    update(&s);
    set_root_name(&s);
    sleep(DS_INTERVAL);
  }
  
  stop_monitors(monitors, nt);
  info_free(&s);
  return 0;
}

static void update(struct info *s)
{
  get_temp(s);
  get_mem(s);
  get_wireless(s);
  get_battery_capacity(s);
  get_cpu_usage(s);
  get_time(s);
}

static void set_root_name(struct info *s)
{
  char name[256];
  snprintf(name, 256, DS_FORMATSTRING, DS_ARGS);
  Display *dpy = XOpenDisplay(NULL);
  XStoreName(dpy, RootWindow(dpy, DefaultScreen(dpy)), name);
  XCloseDisplay(dpy);
}

static void info_create(struct info *s)
{
  s->winfo = malloc(sizeof(struct wireless_info));
  s->ba_status = 'U';
  int i;
  for (i = 0; i < DS_CPU_COUNT+1; i++)
    s->cpu[i].idle = s->cpu[i].nonidle = 0;
}

static void info_free(struct info *s)
{
  free(s->winfo);
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

static void extract_cpu_times(char *line, int count, ...)
{
  va_list args;
  static char *stat;
  unsigned long long *cur;
  char num[21];
  int i, j;

  if (line != NULL)
    stat = line;
  va_start(args, count);
  cur = va_arg(args, unsigned long long *);

  for (i = j = 0; j < count; stat++) {
    switch (*stat) {
    case ' ':
      num[i] = 0;
      *cur = atol(num);
      cur = va_arg(args, unsigned long long *);
      j++;
      i = 0;
      break;
    case '\n':
      num[i] = 0;
      *cur = atol(num);
      stat += 6;
      va_end(args);
      return;
    case 0:
      fprintf(stderr, "The CPU times buffer is too small!\n");
      va_end(args);
      return;
    default:
      num[i++] = *stat;
    }
  }
  va_end(args);
}

static void get_cpu_usage(struct info *s)
{
  unsigned long long usertime, nicetime, systemtime, idletime;
  unsigned long long ioWait, irq, softIrq, steal, guest, guestnice;
  unsigned long long systemalltime, idlealltime, totaltime, virtalltime;
  unsigned long long nonidlealltime, prevnonidle, previdle, prevtotaltime;
  int i;
  char tmp[512];
  int fd = open(DS_STAT, O_RDONLY);
  if (fd != -1) {
      lseek(fd, 5, SEEK_CUR);
      read(fd, tmp, 512);
      close(fd);
  }
  tmp[511] = 0;
  for (i = 0; i < DS_CPU_COUNT + 1; i++) {
    extract_cpu_times(i == 0 ? tmp : NULL, 10, &usertime, &nicetime,
                      &systemtime, &idletime, &ioWait, &irq, &softIrq,
                      &steal, &guest, &guestnice);
    systemalltime = systemtime + irq + softIrq;
    virtalltime = guest + guestnice;
    idlealltime = idletime + ioWait;
    nonidlealltime = usertime + nicetime + systemalltime + steal + virtalltime;
    totaltime = nonidlealltime + idlealltime;

    prevnonidle = s->cpu[i].nonidle;
    previdle = s->cpu[i].idle;
    prevtotaltime = prevnonidle + previdle;
    if (totaltime == prevtotaltime)
      break;
    s->cpu[i].prct = (totaltime + previdle - (prevtotaltime + idlealltime))
      * 100 / (totaltime - prevtotaltime);

    s->cpu[i].nonidle = nonidlealltime;
    s->cpu[i].idle = idlealltime;
  }
}

static void get_battery_capacity(struct info *s)
{
  int fd = open(DS_BATT "capacity", O_RDONLY);
  int n = 0;
  if (fd != -1) {
    n = read(fd, s->ba_capacity, 4);
    close(fd);
  }
  s->ba_capacity[n-1] = 0;
}

static void get_battery_status(struct info *s)
{
  int fd = open(DS_BATT "status", O_RDONLY);
  char c = 'U';
  if (fd != -1) {
    read(fd, &c, 1);
    close(fd);
  }
  if (c == 'U') {
    fd = open(DS_AC "online", O_RDONLY);
    if (fd != -1) {
      read(fd, &c, 1);
      switch (c) {
      case 49:                  /* '1' - on AC */
        c = 'A';
        break;
      case 48:                  /* '0' - on battery */
        c = 'D';
        break;
      }
      close(fd);
    }
  }
  s->ba_status = c;
}

static void get_mem(struct info *s)
{
  struct sysinfo m;
  sysinfo(&m);
  int fd = open(DS_MEM, O_RDONLY);
  char mema[10];
  if (fd != -1) {
    lseek(fd, 73, SEEK_CUR);
    read(fd, mema, 9);
    close(fd);
  }
  mema[9] = 0;
  s->mem_total = m.totalram / 1024;
  s->mem_avail = atol(mema);
}

static void get_temp(struct info *s)
{
  int i, fd;
  char tmp[7];
  for (i = 0; i < DS_TEMP_COUNT; i++) {
    fd = open(DS_TEMP_SENSORS[i], O_RDONLY);
    if (fd != -1) {
      read(fd, tmp, 6);
      close(fd);
    }
    tmp[6] = 0;
    s->temp[i] = atol(tmp) * 0x418938 >> 32;
  }
}

static void get_time(struct info *s)
{
  time_t t;
  time(&t);
  strftime(s->time, 128, DS_TIME_FORMAT, localtime(&t));
}

static void get_wireless(struct info *s)
{
  struct iwreq wrq = {{DS_WIFACE}, {""}};
  int skfd = iw_sockets_open();
  wrq.u.essid.pointer = (caddr_t) s->winfo->b.essid;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  ioctl(skfd, SIOCGIWESSID, &wrq);
  if (ioctl(skfd, SIOCGIWRATE, &wrq) >= 0) {
    memcpy(&(s->winfo->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
  }
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
