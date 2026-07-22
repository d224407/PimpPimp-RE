/*
 * PingPimp – Network Optimizer for Android
 * Dịch từ pseudo-C gốc (PingPimp.c)
 * Build: gcc -static -O2 -o PingPimp PingPimp.c
 * Hoặc dùng NDK: clang -Oz -s -o PingPimp PingPimp.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>

/* ==================== Định nghĩa chung ==================== */

#define MODULE_DIR "/data/adb/modules/PingPimp"
#define MAX_CMD_LEN 1024

/* ----- Ghi log ----- */
static void log_msg(const char *fmt, ...) {
    va_list args;
    char buffer[512];
    FILE *fp;
    time_t t;
    struct tm *tm;
    char time_str[32];

    time(&t);
    tm = localtime(&t);
    strftime(time_str, sizeof(time_str), "[%H:%M:%S]", tm);

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    printf("%s %s\n", time_str, buffer);

    fp = fopen("/data/local/tmp/PingPimp.log", "a");
    if (fp) {
        fprintf(fp, "%s %s\n", time_str, buffer);
        fclose(fp);
    }
}

/* ----- Ghi file sysfs/procfs ----- */
static int write_file(const char *path, const char *value) {
    FILE *fp;
    if (access(path, F_OK) != 0) return -1;
    chmod(path, 0644);
    fp = fopen(path, "w");
    if (!fp) return -1;
    fprintf(fp, "%s\n", value);
    fclose(fp);
    return 0;
}

/* ----- Hàm tiện ích: chạy lệnh system và bỏ qua output ----- */
static void run_cmd(const char *cmd) {
    char buf[MAX_CMD_LEN];
    snprintf(buf, sizeof(buf), "%s >/dev/null 2>&1", cmd);
    system(buf);
}

/* ----- Hàm tiện ích: chạy lệnh và lấy output (popen) ----- */
static char *get_cmd_output(const char *cmd, char *output, size_t size) {
    FILE *fp;
    fp = popen(cmd, "r");
    if (!fp) return NULL;
    if (fgets(output, size, fp)) {
        size_t len = strlen(output);
        while (len > 0 && (output[len-1] == '\n' || output[len-1] == '\r')) {
            output[--len] = '\0';
        }
    } else {
        output[0] = '\0';
    }
    pclose(fp);
    return output;
}

/* ==================== Các chế độ ==================== */

/* ----- 1. Default mode ----- */
static void apply_default(void) {
    write_file("/proc/sys/net/core/netdev_max_backlog", "4096");
    write_file("/proc/sys/net/core/somaxconn", "4096");
    write_file("/proc/sys/net/core/optmem_max", "204800");
    write_file("/proc/sys/net/core/rmem_default", "262144");
    write_file("/proc/sys/net/core/rmem_max", "524288");
    write_file("/proc/sys/net/core/wmem_default", "262144");
    write_file("/proc/sys/net/core/wmem_max", "524288");
    write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    write_file("/proc/sys/net/ipv4/tcp_syn_retries", "4");
    write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 1048576");
    write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 1048576");
    write_file("/proc/sys/net/ipv4/udp_rmem_min", "8192");
    write_file("/proc/sys/net/ipv4/udp_wmem_min", "8192");
    write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "20");
    write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "600");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "15");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    run_cmd("cmd wifi force-low-latency-mode disabled");
    run_cmd("sysctl -w net.ipv4.route.flush=1");
}

/* ----- 2. Game mode (latency ưu tiên) ----- */
static void apply_game(void) {
    write_file("/proc/sys/net/core/netdev_max_backlog", "1000");
    write_file("/proc/sys/net/core/somaxconn", "1024");
    write_file("/proc/sys/net/core/optmem_max", "102400");
    write_file("/proc/sys/net/core/rmem_default", "131072");
    write_file("/proc/sys/net/core/rmem_max", "262144");
    write_file("/proc/sys/net/core/wmem_default", "131072");
    write_file("/proc/sys/net/core/wmem_max", "262144");
    write_file("/proc/sys/net/ipv4/tcp_low_latency", "1");
    write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    write_file("/proc/sys/net/ipv4/tcp_syn_retries", "2");
    write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 65536 262144");
    write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 262144");
    write_file("/proc/sys/net/ipv4/udp_rmem_min", "32768");
    write_file("/proc/sys/net/ipv4/udp_wmem_min", "32768");
    write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "10");
    write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "300");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "10");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "3");
    run_cmd("cmd wifi force-low-latency-mode enabled");
    run_cmd("sysctl -w net.ipv4.route.flush=1");
}

/* ----- 3. Download mode (throughput ưu tiên) ----- */
static void apply_download(void) {
    write_file("/proc/sys/net/core/netdev_max_backlog", "16384");
    write_file("/proc/sys/net/core/somaxconn", "8192");
    write_file("/proc/sys/net/core/optmem_max", "409600");
    write_file("/proc/sys/net/core/rmem_default", "524288");
    write_file("/proc/sys/net/core/rmem_max", "4194304");
    write_file("/proc/sys/net/core/wmem_default", "524288");
    write_file("/proc/sys/net/core/wmem_max", "4194304");
    write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    write_file("/proc/sys/net/ipv4/tcp_syn_retries", "4");
    write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 4194304");
    write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 4194304");
    write_file("/proc/sys/net/ipv4/udp_rmem_min", "8192");
    write_file("/proc/sys/net/ipv4/udp_wmem_min", "8192");
    write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "30");
    write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "1200");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "30");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    run_cmd("sysctl -w net.ipv4.route.flush=1");
}

/* ----- 4. Streaming mode ----- */
static void apply_streaming(void) {
    write_file("/proc/sys/net/core/netdev_max_backlog", "8192");
    write_file("/proc/sys/net/core/somaxconn", "4096");
    write_file("/proc/sys/net/core/optmem_max", "204800");
    write_file("/proc/sys/net/core/rmem_default", "262144");
    write_file("/proc/sys/net/core/rmem_max", "2097152");
    write_file("/proc/sys/net/core/wmem_default", "262144");
    write_file("/proc/sys/net/core/wmem_max", "2097152");
    write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    write_file("/proc/sys/net/ipv4/tcp_syn_retries", "3");
    write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 2097152");
    write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 2097152");
    write_file("/proc/sys/net/ipv4/udp_rmem_min", "16384");
    write_file("/proc/sys/net/ipv4/udp_wmem_min", "16384");
    write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "20");
    write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "1200");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "30");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    run_cmd("sysctl -w net.ipv4.route.flush=1");
}

/* ----- 5. Social mode ----- */
static void apply_social(void) {
    write_file("/proc/sys/net/core/netdev_max_backlog", "4096");
    write_file("/proc/sys/net/core/somaxconn", "4096");
    write_file("/proc/sys/net/core/optmem_max", "204800");
    write_file("/proc/sys/net/core/rmem_default", "131072");
    write_file("/proc/sys/net/core/rmem_max", "524288");
    write_file("/proc/sys/net/core/wmem_default", "131072");
    write_file("/proc/sys/net/core/wmem_max", "524288");
    write_file("/proc/sys/net/ipv4/tcp_low_latency", "1");
    write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    write_file("/proc/sys/net/ipv4/tcp_syn_retries", "3");
    write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 65536 1048576");
    write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 1048576");
    write_file("/proc/sys/net/ipv4/udp_rmem_min", "8192");
    write_file("/proc/sys/net/ipv4/udp_wmem_min", "8192");
    write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "15");
    write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "600");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "15");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    run_cmd("cmd wifi force-low-latency-mode enabled");
    run_cmd("sysctl -w net.ipv4.route.flush=1");
}

/* ----- 6. Browsing mode ----- */
static void apply_browsing(void) {
    write_file("/proc/sys/net/core/netdev_max_backlog", "8192");
    write_file("/proc/sys/net/core/somaxconn", "2048");
    write_file("/proc/sys/net/core/optmem_max", "102400");
    write_file("/proc/sys/net/core/rmem_default", "262144");
    write_file("/proc/sys/net/core/rmem_max", "1048576");
    write_file("/proc/sys/net/core/wmem_default", "262144");
    write_file("/proc/sys/net/core/wmem_max", "1048576");
    write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "1");
    write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    write_file("/proc/sys/net/ipv4/tcp_syn_retries", "5");
    write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 1048576");
    write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 1048576");
    write_file("/proc/sys/net/ipv4/udp_rmem_min", "16384");
    write_file("/proc/sys/net/ipv4/udp_wmem_min", "16384");
    write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "30");
    write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "600");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "15");
    write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    run_cmd("sysctl -w net.ipv4.route.flush=1");
}

/* ----- 7. Outdoor mode ----- */
static void apply_outdoor(void) {
    apply_default();
    run_cmd("cmd wifi force-low-latency-mode enabled");
}

/* ----- 8. State / Unstate ----- */
static void apply_state(void) {
    run_cmd("cmd settings put global netstats_enabled 0");
    run_cmd("cmd wifi set-scan-always-available disabled");
    run_cmd("cmd wifi set-verbose-logging disabled -l 0");
}

static void apply_unstate(void) {
    run_cmd("cmd settings delete global netstats_enabled");
    run_cmd("cmd wifi set-scan-always-available enabled");
    run_cmd("cmd wifi set-verbose-logging enabled -l 1");
}

/* ----- 9. Saver / Unsaver ----- */
static void apply_saver(void) {
    run_cmd("cmd netpolicy set restrict-background true");
    run_cmd("cmd settings put global ingress_rate_limit_bytes_per_second 1048576");
}

static void apply_unsaver(void) {
    run_cmd("cmd netpolicy set restrict-background false");
    run_cmd("cmd settings delete global ingress_rate_limit_bytes_per_second");
}

/* ----- 10. IPv6 Disable / Enable ----- */
static void apply_disable_ipv6(void) {
    run_cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1");
    run_cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1");
    run_cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1");
    run_cmd("content update --uri content://telephony/carriers --bind protocol:s:IP --bind roaming_protocol:s:IP");
    run_cmd("cmd connectivity airplane-mode enable");
    sleep(1);
    run_cmd("cmd connectivity airplane-mode disable");
}

static void apply_enable_ipv6(void) {
    run_cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=0");
    run_cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=0");
    run_cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=0");
    run_cmd("content update --uri content://telephony/carriers --bind protocol:s:IPV4V6 --bind roaming_protocol:s:IPV4V6");
    run_cmd("cmd connectivity airplane-mode enable");
    sleep(1);
    run_cmd("cmd connectivity airplane-mode disable");
}

/* ==================== Hardware Tweak (RPS/XPS) ==================== */

static void apply_hw_tweak(void) {
    FILE *fp;
    DIR *dir;
    struct dirent *ent;
    char cpu_mask[16] = "3";
    char path[PATH_MAX];
    char queues_path[PATH_MAX];
    char interface[64];
    int max_cpu = 3;

    // Đọc /sys/devices/system/cpu/present để lấy số CPU
    fp = fopen("/sys/devices/system/cpu/present", "r");
    if (fp) {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp)) {
            char *dash = strchr(buf, '-');
            if (dash) {
                max_cpu = atoi(dash + 1);
            }
        }
        fclose(fp);
    }

    // Xác định mask cho RPS/XPS dựa trên số CPU
    if (max_cpu >= 7) {
        strcpy(cpu_mask, "ff");
    } else if (max_cpu >= 3) {
        strcpy(cpu_mask, "f");
    } else {
        strcpy(cpu_mask, "3");
    }

    // Set rps_sock_flow_entries
    write_file("/proc/sys/net/core/rps_sock_flow_entries", "32768");

    // Quét các interface mạng
    dir = opendir("/sys/class/net");
    if (!dir) return;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (strncmp(ent->d_name, "lo", 2) == 0) continue;
        if (strncmp(ent->d_name, "dummy", 5) == 0) continue;
        if (strncmp(ent->d_name, "ifb", 3) == 0) continue;

        strcpy(interface, ent->d_name);
        snprintf(path, sizeof(path), "/sys/class/net/%s", interface);
        snprintf(queues_path, sizeof(queues_path), "%s/queues", path);

        DIR *qdir = opendir(queues_path);
        if (!qdir) continue;

        while ((ent = readdir(qdir)) != NULL) {
            if (strncmp(ent->d_name, "rx-", 3) == 0) {
                // rps_cpus
                snprintf(path, sizeof(path), "%s/%s/rps_cpus", queues_path, ent->d_name);
                write_file(path, cpu_mask);
                // rps_flow_cnt
                snprintf(path, sizeof(path), "%s/%s/rps_flow_cnt", queues_path, ent->d_name);
                write_file(path, "4096");
            } else if (strncmp(ent->d_name, "tx-", 3) == 0) {
                // xps_cpus
                snprintf(path, sizeof(path), "%s/%s/xps_cpus", queues_path, ent->d_name);
                write_file(path, cpu_mask);
            }
        }
        closedir(qdir);
    }
    closedir(dir);

    log_msg("HW Tweak applied (CPU mask: %s)", cpu_mask);
}

/* ==================== Traffic Control ==================== */

static void apply_init_tc(void) {
    char interface[64] = "";
    char cmd[MAX_CMD_LEN];

    // Lấy interface mạng mặc định
    get_cmd_output("ip route get 1.1.1.1 2>/dev/null | awk '{print $5}' | head -n1", interface, sizeof(interface));

    if (interface[0] == '\0') return;

    // Xóa qdisc cũ
    snprintf(cmd, sizeof(cmd), "tc qdisc del dev %s root >/dev/null 2>&1", interface);
    system(cmd);

    // Xóa qdisc trên lo
    system("tc qdisc del dev lo root >/dev/null 2>&1");

    // Thử cake trước
    system("tc qdisc add dev lo root cake >/dev/null 2>&1");

    int has_cake = (system("tc qdisc add dev lo root cake >/dev/null 2>&1") == 0);
    system("tc qdisc del dev lo root >/dev/null 2>&1");

    if (has_cake) {
        snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s root handle 1: cake besteffort nat >/dev/null 2>&1", interface);
        system(cmd);
    } else {
        // Fallback: prio + fq_codel
        snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s root handle 1: prio bands 3 priomap 1 2 2 2 1 2 0 0 1 1 1 1 1 1 1 1 >/dev/null 2>&1", interface);
        system(cmd);
        snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s parent 1:1 handle 10: fq_codel quantum 300 limit 1000 flows 1024 noecn >/dev/null 2>&1", interface);
        system(cmd);
        snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s parent 1:2 handle 20: fq_codel quantum 300 limit 1000 flows 1024 noecn >/dev/null 2>&1", interface);
        system(cmd);
        snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s parent 1:3 handle 30: fq_codel quantum 300 limit 1000 flows 1024 noecn >/dev/null 2>&1", interface);
        system(cmd);
    }

    // Filters: MARK 0x40000000/0x40000000 -> flowid 1:1
    snprintf(cmd, sizeof(cmd), "tc filter add dev %s parent 1: protocol ip prio 1 handle 0x40000000/0x40000000 fw flowid 1:1 >/dev/null 2>&1", interface);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "tc filter add dev %s parent 1: protocol ipv6 prio 1 handle 0x40000000/0x40000000 fw flowid 1:1 >/dev/null 2>&1", interface);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "tc filter add dev %s parent 1: protocol ip prio 2 u32 match ip tos 0xb8 0xfc flowid 1:1 >/dev/null 2>&1", interface);
    system(cmd);

    log_msg("Init TC applied on interface: %s", interface);
}

/* ==================== Boost Apps (DSCP/MARK) ==================== */

static void apply_boost_apps(void) {
    FILE *fp;
    char file_path[PATH_MAX];
    char buffer[4096];
    char uid_buf[64];
    char cmd[MAX_CMD_LEN];
    char *line, *ptr;
    int use_mark = 0;

    snprintf(file_path, sizeof(file_path), "%s/boost_apps.txt", MODULE_DIR);

    // Kiểm tra xem iptables DSCP có hỗ trợ không
    int ret = system("iptables -t mangle -A OUTPUT -m owner --uid-owner 99999 -j DSCP --set-dscp 46 >/dev/null 2>&1");
    if (ret == 0) {
        system("iptables -t mangle -D OUTPUT -m owner --uid-owner 99999 -j DSCP --set-dscp 46 >/dev/null 2>&1");
        use_mark = 0;
    } else {
        use_mark = 1;
    }

    fp = fopen(file_path, "r");
    if (!fp) return;

    size_t len = fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);
    buffer[len] = '\0';

    line = strtok(buffer, ",\n\r");
    while (line) {
        // Trim whitespace
        while (*line == ' ') line++;
        char *end = line + strlen(line) - 1;
        while (end > line && (*end == ' ' || *end == '\r' || *end == '\n')) {
            *end = '\0';
            end--;
        }

        if (strlen(line) > 1) {
            // Lấy UID của app
            snprintf(cmd, sizeof(cmd), "pm list packages -U 2>/dev/null | grep -E '^package:%s ' | awk -F'uid:' '{print $2}'", line);
            get_cmd_output(cmd, uid_buf, sizeof(uid_buf));

            if (uid_buf[0] != '\0') {
                if (use_mark) {
                    snprintf(cmd, sizeof(cmd), "iptables -t mangle -I OUTPUT -m owner --uid-owner %s -j MARK --set-mark 0x40000000/0x40000000 >/dev/null 2>&1", uid_buf);
                    system(cmd);
                    snprintf(cmd, sizeof(cmd), "ip6tables -t mangle -I OUTPUT -m owner --uid-owner %s -j MARK --set-mark 0x40000000/0x40000000 >/dev/null 2>&1", uid_buf);
                    system(cmd);
                } else {
                    snprintf(cmd, sizeof(cmd), "iptables -t mangle -I OUTPUT -m owner --uid-owner %s -j DSCP --set-dscp 46 >/dev/null 2>&1", uid_buf);
                    system(cmd);
                    snprintf(cmd, sizeof(cmd), "ip6tables -t mangle -I OUTPUT -m owner --uid-owner %s -j DSCP --set-dscp 46 >/dev/null 2>&1", uid_buf);
                    system(cmd);
                }
                log_msg("Boost app: %s (UID: %s)", line, uid_buf);
            }
        }
        line = strtok(NULL, ",\n\r");
    }
}

/* ==================== Boot Mode ==================== */

static void apply_boot(void) {
    char file_path[PATH_MAX];
    char buf[64];
    char cmd[MAX_CMD_LEN];
    FILE *fp;

    log_msg("Running boot configuration...");

    // 1. Đọc preset.txt
    snprintf(file_path, sizeof(file_path), "%s/preset.txt", MODULE_DIR);
    if (stat(file_path, &(struct stat){0}) == 0) {
        fp = fopen(file_path, "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                // Loại bỏ newline
                size_t len = strlen(buf);
                while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
                    buf[--len] = '\0';
                }
                if (strcasecmp(buf, "game") == 0) {
                    apply_game();
                } else if (strcasecmp(buf, "download") == 0) {
                    apply_download();
                } else if (strcasecmp(buf, "streaming") == 0) {
                    apply_streaming();
                } else if (strcasecmp(buf, "social") == 0) {
                    apply_social();
                } else if (strcasecmp(buf, "browsing") == 0) {
                    apply_browsing();
                } else if (strcasecmp(buf, "outdoor") == 0) {
                    apply_outdoor();
                } else {
                    apply_default();
                }
            }
            fclose(fp);
        }
    } else {
        apply_default();
    }

    // 2. Đọc ipv6_state.txt
    snprintf(file_path, sizeof(file_path), "%s/ipv6_state.txt", MODULE_DIR);
    if (stat(file_path, &(struct stat){0}) == 0) {
        fp = fopen(file_path, "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                if (buf[0] == '1') {
                    apply_disable_ipv6();
                } else {
                    apply_enable_ipv6();
                }
            }
            fclose(fp);
        }
    }

    // 3. Đọc state.txt
    snprintf(file_path, sizeof(file_path), "%s/state.txt", MODULE_DIR);
    if (stat(file_path, &(struct stat){0}) == 0) {
        fp = fopen(file_path, "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                if (buf[0] == '1') {
                    apply_state();
                } else {
                    apply_unstate();
                }
            }
            fclose(fp);
        }
    }

    // 4. Đọc saver.txt
    snprintf(file_path, sizeof(file_path), "%s/saver.txt", MODULE_DIR);
    if (stat(file_path, &(struct stat){0}) == 0) {
        fp = fopen(file_path, "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                if (buf[0] == '1') {
                    apply_saver();
                } else {
                    apply_unsaver();
                }
            }
            fclose(fp);
        }
    }

    // 5. Đọc tcp.txt (congestion control)
    snprintf(file_path, sizeof(file_path), "%s/tcp.txt", MODULE_DIR);
    if (stat(file_path, &(struct stat){0}) == 0) {
        fp = fopen(file_path, "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                size_t len = strlen(buf);
                while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
                    buf[--len] = '\0';
                }
                if (buf[0] != '\0') {
                    write_file("/proc/sys/net/ipv4/tcp_congestion_control", buf);
                }
            }
            fclose(fp);
        }
    }

    // 6. Đọc isolate_apps.txt
    snprintf(file_path, sizeof(file_path), "%s/isolate_apps.txt", MODULE_DIR);
    if (stat(file_path, &(struct stat){0}) == 0) {
        fp = fopen(file_path, "r");
        if (fp) {
            char buffer[4096];
            size_t len = fread(buffer, 1, sizeof(buffer) - 1, fp);
            fclose(fp);
            buffer[len] = '\0';

            char *line = strtok(buffer, ",\n\r");
            while (line) {
                while (*line == ' ') line++;
                char *end = line + strlen(line) - 1;
                while (end > line && (*end == ' ' || *end == '\r' || *end == '\n')) {
                    *end = '\0';
                    end--;
                }
                if (strlen(line) > 1) {
                    char uid_buf[64];
                    snprintf(cmd, sizeof(cmd), "pm list packages -U 2>/dev/null | grep -E '^package:%s ' | awk -F'uid:' '{print $2}'", line);
                    get_cmd_output(cmd, uid_buf, sizeof(uid_buf));
                    if (uid_buf[0] != '\0') {
                        snprintf(cmd, sizeof(cmd), "iptables -I OUTPUT -m owner --uid-owner %s -j REJECT >/dev/null 2>&1", uid_buf);
                        system(cmd);
                        snprintf(cmd, sizeof(cmd), "ip6tables -I OUTPUT -m owner --uid-owner %s -j REJECT >/dev/null 2>&1", uid_buf);
                        system(cmd);
                        log_msg("Isolate app: %s (UID: %s)", line, uid_buf);
                    }
                }
                line = strtok(NULL, ",\n\r");
            }
        }
    }

    // 7. Boost Apps
    apply_boost_apps();

    // 8. WPA Supplicant
    const char *wpa_conf = "/data/misc/wifi/wpa_supplicant.conf";
    if (stat(wpa_conf, &(struct stat){0}) == 0) {
        fp = fopen(wpa_conf, "r");
        int has_p2p = 0, has_ap_scan = 0;
        if (fp) {
            while (fgets(buf, sizeof(buf), fp)) {
                if (strstr(buf, "p2p_disabled=1")) has_p2p = 1;
                if (strstr(buf, "ap_scan=1")) has_ap_scan = 1;
            }
            fclose(fp);
        }
        fp = fopen(wpa_conf, "a");
        if (fp) {
            if (!has_p2p) fwrite("p2p_disabled=1\n", 15, 1, fp);
            if (!has_ap_scan) fwrite("ap_scan=1\n", 10, 1, fp);
            fclose(fp);
        }
    }

    // 9. Power save off
    run_cmd("iw wlan0 set power_save off");

    // 10. Txqueuelen
    DIR *dir = opendir("/sys/class/net");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            int txqueuelen = 1000;
            if (strncmp(ent->d_name, "lo", 2) == 0 || strncmp(ent->d_name, "dummy", 5) == 0) {
                txqueuelen = 100;
            } else if (strncmp(ent->d_name, "wlan", 4) == 0 || strncmp(ent->d_name, "swlan", 5) == 0) {
                txqueuelen = 3000;
            } else if (strncmp(ent->d_name, "rmnet", 5) == 0 || strncmp(ent->d_name, "ccmni", 5) == 0 || strncmp(ent->d_name, "seth", 4) == 0) {
                txqueuelen = 2500;
            } else if (strncmp(ent->d_name, "ipsec", 5) == 0) {
                txqueuelen = 2500;
            }
            snprintf(cmd, sizeof(cmd), "ifconfig %s txqueuelen %d >/dev/null 2>&1", ent->d_name, txqueuelen);
            system(cmd);
            snprintf(cmd, sizeof(cmd), "ip link set %s txqueuelen %d >/dev/null 2>&1", ent->d_name, txqueuelen);
            system(cmd);
        }
        closedir(dir);
    }

    // 11. Stop các dịch vụ logging
    run_cmd("stop tcpdump");
    run_cmd("stop vendor.tcpdump");
    run_cmd("stop cnss_diag");
    run_cmd("stop vendor.cnss_diag");

    // 12. Xóa log wlan
    if (stat("/data/vendor/wlan_logs", &(struct stat){0}) == 0) {
        system("rm -rf /data/vendor/wlan_logs/* >/dev/null 2>&1");
        chmod("/data/vendor/wlan_logs", 0);
    }

    // 13. Run HW Tweak & Init TC
    apply_hw_tweak();
    apply_init_tc();

    // 14. Ghi NetworkState
    snprintf(file_path, sizeof(file_path), "%s/NetworkState", MODULE_DIR);
    fp = fopen(file_path, "w");
    if (fp) {
        fwrite("NONE", 4, 1, fp);
        fclose(fp);
    }

    // 15. Vòng lặp giám sát interface
    pid_t pid = fork();
    if (pid == 0) {
        char current_iface[64] = "";
        char last_iface[64] = "NONE";
        char state_file[PATH_MAX];
        snprintf(state_file, sizeof(state_file), "%s/NetworkState", MODULE_DIR);

        while (1) {
            get_cmd_output("ip route get 1.1.1.1 2>/dev/null | awk '{print $5}' | head -n1", current_iface, sizeof(current_iface));

            if (current_iface[0] != '\0' && strcmp(current_iface, "lo") != 0) {
                if (strcmp(current_iface, last_iface) != 0) {
                    strcpy(last_iface, current_iface);
                    fp = fopen(state_file, "w");
                    if (fp) {
                        fputs(current_iface, fp);
                        fclose(fp);
                    }
                    apply_hw_tweak();
                    apply_init_tc();
                    log_msg("Network interface changed to: %s", current_iface);
                }
            } else {
                if (strcmp(last_iface, "NONE") != 0) {
                    strcpy(last_iface, "NONE");
                    fp = fopen(state_file, "w");
                    if (fp) {
                        fwrite("NONE", 4, 1, fp);
                        fclose(fp);
                    }
                }
            }
            sleep(30);
        }
    }
}

/* ==================== Main ==================== */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mode>\n"
                "Modes: --default --game --download --streaming\n"
                "       --social --browsing --outdoor\n"
                "       --state --unstate --saver --unsaver\n"
                "       --disable --enable\n"
                "       --hw-tweak --init-tc --boot\n", argv[0]);
        return 1;
    }

    const char *mode = argv[1];

    if (strcmp(mode, "--default") == 0) {
        apply_default();
    } else if (strcmp(mode, "--game") == 0) {
        apply_game();
    } else if (strcmp(mode, "--download") == 0) {
        apply_download();
    } else if (strcmp(mode, "--streaming") == 0) {
        apply_streaming();
    } else if (strcmp(mode, "--social") == 0) {
        apply_social();
    } else if (strcmp(mode, "--browsing") == 0) {
        apply_browsing();
    } else if (strcmp(mode, "--outdoor") == 0) {
        apply_outdoor();
    } else if (strcmp(mode, "--state") == 0) {
        apply_state();
    } else if (strcmp(mode, "--unstate") == 0) {
        apply_unstate();
    } else if (strcmp(mode, "--saver") == 0) {
        apply_saver();
    } else if (strcmp(mode, "--unsaver") == 0) {
        apply_unsaver();
    } else if (strcmp(mode, "--disable") == 0) {
        apply_disable_ipv6();
    } else if (strcmp(mode, "--enable") == 0) {
        apply_enable_ipv6();
    } else if (strcmp(mode, "--hw-tweak") == 0) {
        apply_hw_tweak();
    } else if (strcmp(mode, "--init-tc") == 0) {
        apply_init_tc();
    } else if (strcmp(mode, "--boot") == 0) {
        apply_boot();
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        return 1;
    }

    return 0;
}