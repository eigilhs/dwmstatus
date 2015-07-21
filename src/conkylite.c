#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <iwlib.h>
#include <libudev.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "config.h"
#include "conkylite.h"

static volatile sig_atomic_t sig_status;

int main(void)
{
  struct info s;
  struct conky_monitor *monitors;
  unsigned char status[256];
  XTextProperty xtp = {status, XA_STRING, 8, 0};
  int nt;
  info_create(&s);
  signal(SIGINT, catch_sigint);

  update(&s);
  nt = start_monitors(&s, &monitors);
  while (!sig_status) {
    update(&s);
    set_root_name(&s, xtp);
    sleep(CL_INTERVAL);
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

static void set_root_name(struct info *s, XTextProperty xtp)
{
  int n = snprintf((char *)xtp.value, 256, CL_FORMATSTRING, CL_ARGS);
  Display *dpy = XOpenDisplay(NULL);
  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);
  xtp.nitems = n;
  XSetTextProperty(dpy, root, &xtp, XA_WM_NAME);
  XCloseDisplay(dpy);
}

static void info_create(struct info *s)
{
  s->winfo = malloc(sizeof(struct wireless_info));
  s->ba_status = 'U';
  int i;
  for (i = 0; i < CL_CPU_COUNT+1; i++)
    s->cpu[i].idle = s->cpu[i].nonidle = 0;
}

static void info_free(struct info *s)
{
  free(s->winfo);
}

static void get_cpu_usage(struct info *s)
{
  unsigned long long usertime, nicetime, systemtime, idletime;
  unsigned long long ioWait, irq, softIrq, steal, guest, guestnice;
  unsigned long long systemalltime, idlealltime, totaltime, virtalltime;
  unsigned long long nonidlealltime, prevnonidle, previdle, prevtotaltime;
  int i;
  char tmp[256];
  FILE *fp = fopen(CL_STAT, "r");
  for (i = 0; i < CL_CPU_COUNT + 1; i++) {
    ioWait = irq = softIrq = steal = guest = guestnice = 0;
    if (fp) {
      fseek(fp, 5, SEEK_CUR);
      fgets(tmp, 255, fp);
      sscanf(tmp, "%llu %llu %llu %llu %llu %llu %llu "
             "%llu %llu %llu", &usertime, &nicetime, &systemtime,
             &idletime, &ioWait, &irq, &softIrq, &steal, &guest, &guestnice);
    } else {
      fprintf(stderr, "Couldn't read %s\n", CL_STAT);
      break;
    }
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
  if (fp)
    fclose(fp);
}

static void get_battery_capacity(struct info *s)
{
  int fd = open(CL_BATT "capacity", O_RDONLY);
  int n = 0;
  if (fd != -1) {
    n = read(fd, s->ba_capacity, 3);
    close(fd);
  }
  s->ba_capacity[n] = 0;
}

static void get_battery_status(struct info *s)
{
  int fd = open(CL_BATT "status", O_RDONLY);
  char c = 'U';
  if (fd != -1) {
    read(fd, &c, 1);
    close(fd);
  }
  if (c == 'U') {
    fd = open(CL_AC "online", O_RDONLY);
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
  int fd = open(CL_MEM, O_RDONLY), n = 0;
  char mema[10], memt[10];
  if (fd != -1) {
    lseek(fd, 15, SEEK_CUR);
    n = read(fd, memt, 9);
    memt[n] = 0;
    lseek(fd, 49, SEEK_CUR);
    read(fd, mema, 9);
    mema[n] = 0;
    close(fd);
  }
  s->mem_total = atoi(memt);
  s->mem_avail = atoi(mema);
}

static void get_temp(struct info *s)
{
  int i, fd;
  char tmp[7] = "0";
  for (i = 0; i < CL_TEMP_COUNT; i++) {
    fd = open(CL_TEMP_SENSORS[i], O_RDONLY);
    if (fd != -1) {
      read(fd, tmp, 6);
      close(fd);
    }
    s->temp[i] = atol(tmp) * 0x418938 >> 32;
  }
}

static void get_time(struct info *s)
{
  time_t t;
  time(&t);
  strftime(s->time, 128, CL_TIME_FORMAT, localtime(&t));
}

static void get_wireless(struct info *s)
{
  struct iwreq wrq;
  int skfd = iw_sockets_open();
  char iface[] = CL_WIFACE;
  wrq.u.essid.pointer = (caddr_t) s->winfo->b.essid;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  iw_get_ext(skfd, iface, SIOCGIWESSID, &wrq);
  if (iw_get_ext(skfd, iface, SIOCGIWRATE, &wrq) >= 0) {
    memcpy(&(s->winfo->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
  }
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
