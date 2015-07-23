#define DS_STAT "/proc/stat"
#define DS_MEM  "/proc/meminfo"
#define DS_BATT "/sys/class/power_supply/BAT0/"
#define DS_AC   "/sys/class/power_supply/AC/"

#define DS_WIFACE "wlan0"
#define DS_INTERVAL 1000000 /* Update every DS_INTERVAL microseconds */

#define DS_TIME_FORMAT "%m-%V-%d %H:%M:%S"

#define DS_FORMATSTRING "%u / %u / %u / %u :: %d / %d / %d °C :: %.2f G :: %s %d :: %c %s %% :: %s"
#define DS_ARGS info->cpu[1].prct, info->cpu[2].prct, info->cpu[3].prct,    \
    info->cpu[4].prct, info->temp[0], info->temp[1], info->temp[2],         \
    (info->mem_total - info->mem_avail) / (float) 0x100000, info->wi_essid, \
    info->wi_bitrate / 80000000, info->ba_status, info->ba_capacity, info->time

/* == CPU ==
 * CPU load info is stored in info->cpu[0].prct through info->cpu[N+1].prct
 * where N is the machine's number of CPUs. info->cpu[0].prct is the total,
 * and the rest are for specific CPUs.
 *
 * == MEMORY ==
 * info->mem_total holds total memory installed, and info->mem_avail holds
 * the amount of "available" memory.
 *
 * == TEMPERATURE ==
 * The value of the temperature sensors defined in DS_TEMP_SENSORS can be
 * read from info->temp[i] where "i" is the index of the sensor.
 *
 * == WIFI ==
 * info->wi_essid holds the current ESSID, and info->wi_bitrate holds the
 * bitrate.
 *
 * == POWER ==
 * info->ba_status holds 'C' when charging, 'D' when discharging, 'F' when
 * fully charged, and 'A' when on AC but not charging.
 *
 * == TIME ==
 * A string with current localtime formatted as described in DS_TIME_FORMAT
 * can be found in info->time.
 */
