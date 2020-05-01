#include <libnotify/notify.h>
#include <stdlib.h>
#include <signal.h>

static NotifyNotification *low_battery;
static volatile sig_atomic_t running;

static void
notifier_init() {
    notify_init("Battery notifier");
    low_battery = notify_notification_new("Battery", "low battery", NULL);
    notify_notification_set_timeout(low_battery, NOTIFY_EXPIRES_NEVER);
    notify_notification_set_urgency(low_battery, NOTIFY_URGENCY_CRITICAL);
}

static void
notifier_deinit() {
    g_object_unref(G_OBJECT(low_battery));
    notify_uninit();
}

static float get_battery_full() {
    float energy_full = 0.0f;
    FILE *fd = fopen("/sys/class/power_supply/BAT0/energy_full", "r");
    if (fd) {
        fscanf(fd, "%f", &energy_full);
        fclose(fd);
    }
    return energy_full;
}

static float get_battery_level(float max) {
    float energy_now = 0.0f;
    FILE *fd = fopen("/sys/class/power_supply/BAT0/energy_now", "r");
    if (fd) {
        fscanf(fd, "%f", &energy_now);
        fclose(fd);
    }
    return 100.0 * energy_now / max;
}

static int is_ac_on() {
    int result = -1;
    FILE *fd = fopen("/sys/class/power_supply/AC/online", "r");
    if (fd) {
        fscanf(fd, "%d", &result);
        fclose(fd);
    }
    return result;
}

static void
signal_handler(int signum) {
    running = 0;
}

int main() {
    float level;
    float energy_full = get_battery_full();
    int close_popup = 0;

    notifier_init();
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);



    running = 1;
    while (running) {
        if (!is_ac_on()) {
            level = get_battery_level(energy_full);
            if (level < 15.0f) { // less 15%
                notify_notification_close(low_battery, NULL);
                notify_notification_show(low_battery, NULL);
                close_popup = 1;
            }
        } else {
            if (close_popup) {
                notify_notification_close(low_battery, NULL);
                close_popup = 0;
            }
        }
        sleep(1);
    }

    notifier_deinit();
    return 0;
}
