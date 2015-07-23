#define DS_STAT "/proc/stat"
#define DS_MEM  "/proc/meminfo"
#define DS_BATT "/sys/class/power_supply/BAT0/"
#define DS_AC   "/sys/class/power_supply/AC/"

#define DS_WIFACE "wlan0"
#define DS_INTERVAL 1           /* Update every INTERVAL seconds */

#define DS_TEMP_COUNT 3
static char *DS_TEMP_SENSORS[DS_TEMP_COUNT] = {"/sys/class/hwmon/hwmon0/temp1_input",
                                               "/sys/class/hwmon/hwmon2/temp2_input",
                                               "/sys/class/hwmon/hwmon2/temp4_input"};

#define DS_TIME_FORMAT "%m-%V-%d %H:%M:%S"

#define DS_FORMATSTRING "%u / %u / %u / %u :: %d / %d / %d °C :: %.2f G :: %s %llu :: %c %s %% :: %s"
#define DS_ARGS s->cpu[1].prct, s->cpu[2].prct, s->cpu[3].prct, s->cpu[4].prct,    \
    s->temp[0], s->temp[1], s->temp[2],                                            \
    (s->mem_total - s->mem_avail) / (float) 0x100000,                              \
    s->winfo->b.essid, (unsigned long long) s->winfo->bitrate.value * 0x219 >> 32, \
    s->ba_status, s->ba_capacity, s->time

/* == CPU ==
 * CPU load info is stored in s->cpu[0].prct through s->cpu[N+1].prct
 * where N is the machine's number of CPUs. s->cpu[0].prct is the
 * total, and the rest are for specific CPUs.
 *
 * == MEMORY ==
 * s->mem_total holds total memory installed, and s->mem_avail holds
 * the amount of "available" memory.
 *
 * == TEMPERATURE ==
 * The value of the temperature sensors defined in DS_TEMP_SENSORS
 * can be read from s->temp[i] where "i" is the index of the sensor.
 *
 * == WIFI ==
 * s->winfo->b.essid holds the current ESSID, and s->winfo->bitrate->value
 * holds the bitrate.
 *
 * == POWER ==
 * s->ba_status holds 'C' when charging, 'D' when discharging, 'F' when
 * fully charged, and 'A' when on AC but not charging.
 *
 * == TIME ==
 * A string with current localtime formatted as described in DS_TIME_FORMAT
 * can be found in s->time.
 */
