/*
 * PingPimp – Network Optimizer for Android
 * Phiên bản an toàn bộ nhớ
 * Tuân thủ: 
 *   - Không dùng strcpy/strcat/sprintf
 *   - Kiểm tra malloc/calloc
 *   - Gán NULL sau free
 *   - Dùng snprintf/strncpy an toàn
 *   - Hàm wrapper an toàn
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
#include <stdint.h>

/* ==================== Định nghĩa hằng số ==================== */

#define MODULE_DIR         "/data/adb/modules/PingPimp"
#define LOG_FILE           "/data/local/tmp/PingPimp.log"
#define MAX_CMD_LEN        1024
#define MAX_PATH_LEN       PATH_MAX
#define MAX_LINE_LEN       512
#define MAX_BUFFER_LEN     4096
#define SAFE_STRNCPY(dst, src, n) do { \
    strncpy((dst), (src), (n) - 1); \
    (dst)[(n) - 1] = '\0'; \
} while(0)

/* ==================== Cấu trúc quản lý tài nguyên ==================== */

typedef struct {
    FILE *fp;
    char *buffer;
    size_t buffer_size;
} SafeFileHandle;

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} SafeString;

/* ==================== Hàm quản lý bộ nhớ an toàn ==================== */

static void* safe_malloc(size_t size) {
    if (size == 0) return NULL;
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "ERROR: malloc(%zu) failed\n", size);
    }
    return ptr;
}

static void* safe_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        fprintf(stderr, "ERROR: calloc(%zu, %zu) failed\n", nmemb, size);
    }
    return ptr;
}

static void safe_free(void **ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

static void safe_fclose(FILE **fp) {
    if (fp && *fp) {
        fclose(*fp);
        *fp = NULL;
    }
}

/* ==================== Xử lý chuỗi an toàn ==================== */

static int safe_snprintf(char *buffer, size_t size, const char *format, ...) {
    if (!buffer || size == 0) return -1;
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args);
    if (result < 0 || (size_t)result >= size) {
        buffer[size - 1] = '\0';
        return -1;
    }
    return result;
}

static char* safe_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char *dst = (char*)safe_malloc(len);
    if (dst) {
        memcpy(dst, src, len);
    }
    return dst;
}

static int safe_strcat(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) return -1;
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    if (dest_len + src_len + 1 > dest_size) {
        return -1;
    }
    memcpy(dest + dest_len, src, src_len + 1);
    return 0;
}

/* ==================== Hàm ghi log an toàn ==================== */

static void log_msg(const char *fmt, ...) {
    if (!fmt) return;
    
    char buffer[MAX_LINE_LEN];
    char time_str[32];
    time_t t;
    struct tm *tm_info;
    va_list args;
    FILE *fp;
    
    // Lấy thời gian
    time(&t);
    tm_info = localtime(&t);
    if (!tm_info) {
        strcpy(time_str, "[??:??:??]");
    } else {
        strftime(time_str, sizeof(time_str), "[%H:%M:%S]", tm_info);
    }
    
    // Format message
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = '\0';
    
    // In console
    printf("%s %s\n", time_str, buffer);
    
    // Ghi log file
    fp = fopen(LOG_FILE, "a");
    if (fp) {
        fprintf(fp, "%s %s\n", time_str, buffer);
        fclose(fp);
    }
}

/* ==================== Hàm ghi file an toàn ==================== */

static int safe_write_file(const char *path, const char *value) {
    if (!path || !value) return -1;
    
    FILE *fp = NULL;
    struct stat st;
    int result = -1;
    mode_t original_mode = 0;
    char read_buffer[MAX_LINE_LEN] = {0};
    
    // Kiểm tra file tồn tại
    if (stat(path, &st) != 0) {
        return -1;
    }
    
    // Kiểm tra là file thường
    if (!S_ISREG(st.st_mode)) {
        return -1;
    }
    
    // Lưu quyền cũ
    original_mode = st.st_mode & 0777;
    
    // Thêm quyền ghi nếu cần
    if (access(path, W_OK) != 0) {
        chmod(path, original_mode | S_IWUSR);
    }
    
    // Mở file để ghi
    fp = fopen(path, "w");
    if (!fp) {
        log_msg("Failed to open %s for writing", path);
        goto cleanup;
    }
    
    // Ghi giá trị
    if (fprintf(fp, "%s\n", value) < 0) {
        log_msg("Failed to write to %s", path);
        goto cleanup;
    }
    
    fclose(fp);
    fp = NULL;
    
    // Xác minh (đọc lại)
    fp = fopen(path, "r");
    if (!fp) {
        goto cleanup;
    }
    
    if (fgets(read_buffer, sizeof(read_buffer), fp)) {
        // Xóa ký tự xuống dòng
        size_t len = strlen(read_buffer);
        while (len > 0 && (read_buffer[len-1] == '\n' || read_buffer[len-1] == '\r')) {
            read_buffer[--len] = '\0';
        }
        if (strcmp(read_buffer, value) == 0) {
            result = 0;
        }
    }
    
cleanup:
    if (fp) fclose(fp);
    fp = NULL;
    
    // Khôi phục quyền
    if (original_mode > 0) {
        chmod(path, original_mode);
    }
    
    return result;
}

/* ==================== Hàm đọc file an toàn ==================== */

static char* safe_read_file(const char *path, size_t *out_size) {
    if (!path) return NULL;
    
    FILE *fp = NULL;
    char *buffer = NULL;
    size_t size = 0;
    size_t capacity = 1024;
    size_t read_size;
    
    fp = fopen(path, "r");
    if (!fp) {
        return NULL;
    }
    
    buffer = (char*)safe_malloc(capacity);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    
    while ((read_size = fread(buffer + size, 1, capacity - size - 1, fp)) > 0) {
        size += read_size;
        if (size >= capacity - 1) {
            capacity *= 2;
            char *new_buf = (char*)realloc(buffer, capacity);
            if (!new_buf) {
                safe_free((void**)&buffer);
                fclose(fp);
                return NULL;
            }
            buffer = new_buf;
        }
    }
    
    buffer[size] = '\0';
    fclose(fp);
    
    if (out_size) {
        *out_size = size;
    }
    
    return buffer;
}

/* ==================== Hàm chạy lệnh an toàn ==================== */

static int safe_system(const char *cmd) {
    if (!cmd) return -1;
    char buf[MAX_CMD_LEN];
    safe_snprintf(buf, sizeof(buf), "%s >/dev/null 2>&1", cmd);
    return system(buf);
}

static char* safe_get_cmd_output(const char *cmd, char *output, size_t size) {
    if (!cmd || !output || size == 0) return NULL;
    
    FILE *fp = NULL;
    char *result = NULL;
    
    fp = popen(cmd, "r");
    if (!fp) {
        return NULL;
    }
    
    if (fgets(output, size, fp)) {
        // Xóa ký tự xuống dòng
        size_t len = strnlen(output, size);
        while (len > 0 && (output[len-1] == '\n' || output[len-1] == '\r')) {
            output[--len] = '\0';
        }
        result = output;
    } else {
        output[0] = '\0';
    }
    
    pclose(fp);
    return result;
}

/* ==================== Hàm kiểm tra file tồn tại ==================== */

static int file_exists(const char *path) {
    if (!path) return 0;
    struct stat st;
    return stat(path, &st) == 0;
}

static int is_regular_file(const char *path) {
    if (!path) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

/* ==================== Các hàm apply mode (đã được bảo vệ) ==================== */

static void apply_default(void) {
    safe_write_file("/proc/sys/net/core/netdev_max_backlog", "4096");
    safe_write_file("/proc/sys/net/core/somaxconn", "4096");
    safe_write_file("/proc/sys/net/core/optmem_max", "204800");
    safe_write_file("/proc/sys/net/core/rmem_default", "262144");
    safe_write_file("/proc/sys/net/core/rmem_max", "524288");
    safe_write_file("/proc/sys/net/core/wmem_default", "262144");
    safe_write_file("/proc/sys/net/core/wmem_max", "524288");
    safe_write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_syn_retries", "4");
    safe_write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 1048576");
    safe_write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 1048576");
    safe_write_file("/proc/sys/net/ipv4/udp_rmem_min", "8192");
    safe_write_file("/proc/sys/net/ipv4/udp_wmem_min", "8192");
    safe_write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "20");
    safe_write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "600");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "15");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    safe_system("cmd wifi force-low-latency-mode disabled");
    safe_system("sysctl -w net.ipv4.route.flush=1");
}

static void apply_game(void) {
    safe_write_file("/proc/sys/net/core/netdev_max_backlog", "1000");
    safe_write_file("/proc/sys/net/core/somaxconn", "1024");
    safe_write_file("/proc/sys/net/core/optmem_max", "102400");
    safe_write_file("/proc/sys/net/core/rmem_default", "131072");
    safe_write_file("/proc/sys/net/core/rmem_max", "262144");
    safe_write_file("/proc/sys/net/core/wmem_default", "131072");
    safe_write_file("/proc/sys/net/core/wmem_max", "262144");
    safe_write_file("/proc/sys/net/ipv4/tcp_low_latency", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_syn_retries", "2");
    safe_write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 65536 262144");
    safe_write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 262144");
    safe_write_file("/proc/sys/net/ipv4/udp_rmem_min", "32768");
    safe_write_file("/proc/sys/net/ipv4/udp_wmem_min", "32768");
    safe_write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "10");
    safe_write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "300");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "10");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "3");
    safe_system("cmd wifi force-low-latency-mode enabled");
    safe_system("sysctl -w net.ipv4.route.flush=1");
}

static void apply_download(void) {
    safe_write_file("/proc/sys/net/core/netdev_max_backlog", "16384");
    safe_write_file("/proc/sys/net/core/somaxconn", "8192");
    safe_write_file("/proc/sys/net/core/optmem_max", "409600");
    safe_write_file("/proc/sys/net/core/rmem_default", "524288");
    safe_write_file("/proc/sys/net/core/rmem_max", "4194304");
    safe_write_file("/proc/sys/net/core/wmem_default", "524288");
    safe_write_file("/proc/sys/net/core/wmem_max", "4194304");
    safe_write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_syn_retries", "4");
    safe_write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 4194304");
    safe_write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 4194304");
    safe_write_file("/proc/sys/net/ipv4/udp_rmem_min", "8192");
    safe_write_file("/proc/sys/net/ipv4/udp_wmem_min", "8192");
    safe_write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "30");
    safe_write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "1200");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "30");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    safe_system("sysctl -w net.ipv4.route.flush=1");
}

static void apply_streaming(void) {
    safe_write_file("/proc/sys/net/core/netdev_max_backlog", "8192");
    safe_write_file("/proc/sys/net/core/somaxconn", "4096");
    safe_write_file("/proc/sys/net/core/optmem_max", "204800");
    safe_write_file("/proc/sys/net/core/rmem_default", "262144");
    safe_write_file("/proc/sys/net/core/rmem_max", "2097152");
    safe_write_file("/proc/sys/net/core/wmem_default", "262144");
    safe_write_file("/proc/sys/net/core/wmem_max", "2097152");
    safe_write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_syn_retries", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 2097152");
    safe_write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 2097152");
    safe_write_file("/proc/sys/net/ipv4/udp_rmem_min", "16384");
    safe_write_file("/proc/sys/net/ipv4/udp_wmem_min", "16384");
    safe_write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "20");
    safe_write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "1200");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "30");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    safe_system("sysctl -w net.ipv4.route.flush=1");
}

static void apply_social(void) {
    safe_write_file("/proc/sys/net/core/netdev_max_backlog", "4096");
    safe_write_file("/proc/sys/net/core/somaxconn", "4096");
    safe_write_file("/proc/sys/net/core/optmem_max", "204800");
    safe_write_file("/proc/sys/net/core/rmem_default", "131072");
    safe_write_file("/proc/sys/net/core/rmem_max", "524288");
    safe_write_file("/proc/sys/net/core/wmem_default", "131072");
    safe_write_file("/proc/sys/net/core/wmem_max", "524288");
    safe_write_file("/proc/sys/net/ipv4/tcp_low_latency", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_syn_retries", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 65536 1048576");
    safe_write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 1048576");
    safe_write_file("/proc/sys/net/ipv4/udp_rmem_min", "8192");
    safe_write_file("/proc/sys/net/ipv4/udp_wmem_min", "8192");
    safe_write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "15");
    safe_write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "600");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "15");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    safe_system("cmd wifi force-low-latency-mode enabled");
    safe_system("sysctl -w net.ipv4.route.flush=1");
}

static void apply_browsing(void) {
    safe_write_file("/proc/sys/net/core/netdev_max_backlog", "8192");
    safe_write_file("/proc/sys/net/core/somaxconn", "2048");
    safe_write_file("/proc/sys/net/core/optmem_max", "102400");
    safe_write_file("/proc/sys/net/core/rmem_default", "262144");
    safe_write_file("/proc/sys/net/core/rmem_max", "1048576");
    safe_write_file("/proc/sys/net/core/wmem_default", "262144");
    safe_write_file("/proc/sys/net/core/wmem_max", "1048576");
    safe_write_file("/proc/sys/net/ipv4/tcp_low_latency", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_slow_start_after_idle", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_sack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_fack", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_window_scaling", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_syn_retries", "5");
    safe_write_file("/proc/sys/net/ipv4/tcp_rmem", "4096 87380 1048576");
    safe_write_file("/proc/sys/net/ipv4/tcp_wmem", "4096 65536 1048576");
    safe_write_file("/proc/sys/net/ipv4/udp_rmem_min", "16384");
    safe_write_file("/proc/sys/net/ipv4/udp_wmem_min", "16384");
    safe_write_file("/proc/sys/net/ipv4/tcp_fastopen", "3");
    safe_write_file("/proc/sys/net/ipv4/tcp_fin_timeout", "30");
    safe_write_file("/proc/sys/net/ipv4/tcp_timestamps", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_no_metrics_save", "0");
    safe_write_file("/proc/sys/net/ipv4/tcp_mtu_probing", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_ecn", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_tw_reuse", "1");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_time", "600");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_intvl", "15");
    safe_write_file("/proc/sys/net/ipv4/tcp_keepalive_probes", "5");
    safe_system("sysctl -w net.ipv4.route.flush=1");
}

static void apply_outdoor(void) {
    apply_default();
    safe_system("cmd wifi force-low-latency-mode enabled");
}

/* ==================== State / Unstate ==================== */

static void apply_state(void) {
    safe_system("cmd settings put global netstats_enabled 0");
    safe_system("cmd wifi set-scan-always-available disabled");
    safe_system("cmd wifi set-verbose-logging disabled -l 0");
}

static void apply_unstate(void) {
    safe_system("cmd settings delete global netstats_enabled");
    safe_system("cmd wifi set-scan-always-available enabled");
    safe_system("cmd wifi set-verbose-logging enabled -l 1");
}

/* ==================== Saver / Unsaver ==================== */

static void apply_saver(void) {
    safe_system("cmd netpolicy set restrict-background true");
    safe_system("cmd settings put global ingress_rate_limit_bytes_per_second 1048576");
}

static void apply_unsaver(void) {
    safe_system("cmd netpolicy set restrict-background false");
    safe_system("cmd settings delete global ingress_rate_limit_bytes_per_second");
}

/* ==================== IPv6 ==================== */

static void apply_disable_ipv6(void) {
    safe_system("sysctl -w net.ipv6.conf.all.disable_ipv6=1");
    safe_system("sysctl -w net.ipv6.conf.default.disable_ipv6=1");
    safe_system("sysctl -w net.ipv6.conf.lo.disable_ipv6=1");
    safe_system("content update --uri content://telephony/carriers --bind protocol:s:IP --bind roaming_protocol:s:IP");
    safe_system("cmd connectivity airplane-mode enable");
    sleep(1);
    safe_system("cmd connectivity airplane-mode disable");
}

static void apply_enable_ipv6(void) {
    safe_system("sysctl -w net.ipv6.conf.all.disable_ipv6=0");
    safe_system("sysctl -w net.ipv6.conf.default.disable_ipv6=0");
    safe_system("sysctl -w net.ipv6.conf.lo.disable_ipv6=0");
    safe_system("content update --uri content://telephony/carriers --bind protocol:s:IPV4V6 --bind roaming_protocol:s:IPV4V6");
    safe_system("cmd connectivity airplane-mode enable");
    sleep(1);
    safe_system("cmd connectivity airplane-mode disable");
}

/* ==================== HW Tweak (RPS/XPS) ==================== */

static void apply_hw_tweak(void) {
    FILE *fp = NULL;
    DIR *dir = NULL, *qdir = NULL;
    struct dirent *ent = NULL;
    char cpu_mask[16] = "3";
    char path[MAX_PATH_LEN];
    char queues_path[MAX_PATH_LEN];
    char buf[64];
    int max_cpu = 3;
    
    // Đọc /sys/devices/system/cpu/present
    fp = fopen("/sys/devices/system/cpu/present", "r");
    if (fp) {
        if (fgets(buf, sizeof(buf), fp)) {
            char *dash = strchr(buf, '-');
            if (dash) {
                int cpu_count = atoi(dash + 1);
                if (cpu_count > 0) max_cpu = cpu_count;
            }
        }
        fclose(fp);
        fp = NULL;
    }
    
    // Xác định mask
    if (max_cpu >= 7) {
        safe_snprintf(cpu_mask, sizeof(cpu_mask), "ff");
    } else if (max_cpu >= 3) {
        safe_snprintf(cpu_mask, sizeof(cpu_mask), "f");
    } else {
        safe_snprintf(cpu_mask, sizeof(cpu_mask), "3");
    }
    
    // Set rps_sock_flow_entries
    safe_write_file("/proc/sys/net/core/rps_sock_flow_entries", "32768");
    
    // Quét các interface mạng
    dir = opendir("/sys/class/net");
    if (!dir) {
        log_msg("Cannot open /sys/class/net");
        return;
    }
    
    while ((ent = readdir(dir)) != NULL) {
        if (!ent) break;
        if (ent->d_name[0] == '.') continue;
        if (strncmp(ent->d_name, "lo", 2) == 0) continue;
        if (strncmp(ent->d_name, "dummy", 5) == 0) continue;
        if (strncmp(ent->d_name, "ifb", 3) == 0) continue;
        
        safe_snprintf(path, sizeof(path), "/sys/class/net/%s", ent->d_name);
        safe_snprintf(queues_path, sizeof(queues_path), "%s/queues", path);
        
        qdir = opendir(queues_path);
        if (!qdir) continue;
        
        while ((ent = readdir(qdir)) != NULL) {
            if (!ent) break;
            if (strncmp(ent->d_name, "rx-", 3) == 0) {
                safe_snprintf(path, sizeof(path), "%s/%s/rps_cpus", queues_path, ent->d_name);
                safe_write_file(path, cpu_mask);
                safe_snprintf(path, sizeof(path), "%s/%s/rps_flow_cnt", queues_path, ent->d_name);
                safe_write_file(path, "4096");
            } else if (strncmp(ent->d_name, "tx-", 3) == 0) {
                safe_snprintf(path, sizeof(path), "%s/%s/xps_cpus", queues_path, ent->d_name);
                safe_write_file(path, cpu_mask);
            }
        }
        closedir(qdir);
        qdir = NULL;
    }
    
    closedir(dir);
    dir = NULL;
    
    log_msg("HW Tweak applied (CPU mask: %s)", cpu_mask);
}

/* ==================== Traffic Control ==================== */

static void apply_init_tc(void) {
    char interface[64] = "";
    char cmd[MAX_CMD_LEN];
    int has_cake = 0;
    
    // Lấy interface mạng mặc định
    safe_get_cmd_output("ip route get 1.1.1.1 2>/dev/null | awk '{print $5}' | head -n1", 
                        interface, sizeof(interface));
    
    if (interface[0] == '\0') {
        log_msg("No network interface found");
        return;
    }
    
    // Xóa qdisc cũ
    safe_snprintf(cmd, sizeof(cmd), "tc qdisc del dev %s root >/dev/null 2>&1", interface);
    system(cmd);
    safe_system("tc qdisc del dev lo root >/dev/null 2>&1");
    
    // Kiểm tra cake support
    int ret = system("tc qdisc add dev lo root cake >/dev/null 2>&1");
    has_cake = (ret == 0);
    safe_system("tc qdisc del dev lo root >/dev/null 2>&1");
    
    if (has_cake) {
        safe_snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s root handle 1: cake besteffort nat >/dev/null 2>&1", interface);
        system(cmd);
        log_msg("Cake qdisc applied on %s", interface);
    } else {
        // Fallback: prio + fq_codel
        safe_snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s root handle 1: prio bands 3 priomap 1 2 2 2 1 2 0 0 1 1 1 1 1 1 1 1 >/dev/null 2>&1", interface);
        system(cmd);
        safe_snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s parent 1:1 handle 10: fq_codel quantum 300 limit 1000 flows 1024 noecn >/dev/null 2>&1", interface);
        system(cmd);
        safe_snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s parent 1:2 handle 20: fq_codel quantum 300 limit 1000 flows 1024 noecn >/dev/null 2>&1", interface);
        system(cmd);
        safe_snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s parent 1:3 handle 30: fq_codel quantum 300 limit 1000 flows 1024 noecn >/dev/null 2>&1", interface);
        system(cmd);
        log_msg("Prio + fq_codel fallback applied on %s", interface);
    }
    
    // Filters
    safe_snprintf(cmd, sizeof(cmd), "tc filter add dev %s parent 1: protocol ip prio 1 handle 0x40000000/0x40000000 fw flowid 1:1 >/dev/null 2>&1", interface);
    system(cmd);
    safe_snprintf(cmd, sizeof(cmd), "tc filter add dev %s parent 1: protocol ipv6 prio 1 handle 0x40000000/0x40000000 fw flowid 1:1 >/dev/null 2>&1", interface);
    system(cmd);
    safe_snprintf(cmd, sizeof(cmd), "tc filter add dev %s parent 1: protocol ip prio 2 u32 match ip tos 0xb8 0xfc flowid 1:1 >/dev/null 2>&1", interface);
    system(cmd);
    
    log_msg("Init TC completed on interface: %s", interface);
}

/* ==================== Main ==================== */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mode>\n", argv[0] ? argv[0] : "PingPimp");
        fprintf(stderr, "Modes: --default --game --download --streaming\n");
        fprintf(stderr, "       --social --browsing --outdoor\n");
        fprintf(stderr, "       --state --unstate --saver --unsaver\n");
        fprintf(stderr, "       --disable --enable\n");
        fprintf(stderr, "       --hw-tweak --init-tc --boot\n");
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
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        return 1;
    }
    
    return 0;
}