#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include "tempcap.h"

static int
read_int(const char *path, int *val)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    int r = fscanf(fp, "%d", val);
    fclose(fp);
    return r == 1 ? 0 : -1;
}

static int
write_int(const char *path, int val)
{
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;
    int r = fprintf(fp, "%d", val);
    fclose(fp);
    return r < 0 ? -1 : 0;
}

static int
sysfs_walk(const char *base, char *out, size_t outsz)
{
    DIR *d = opendir(base);
    if (!d) return -1;

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;

        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", base, e->d_name);

        if (e->d_type == DT_DIR) {
            if (sysfs_walk(full, out, outsz) == 0) {
                closedir(d);
                return 0;
            }
        } else if (e->d_type == DT_REG || e->d_type == DT_LNK) {
            if (strcmp(e->d_name, "tcc_offset_degree_celsius") == 0) {
                snprintf(out, outsz, "%s", full);
                closedir(d);
                return 0;
            }
        }
    }

    closedir(d);
    return -1;
}

int
tcc_discover(struct tcc_info *info)
{
    memset(info, 0, sizeof(*info));

    if (sysfs_walk("/sys/devices", info->sysfs_path, sizeof(info->sysfs_path)) != 0)
        return -1;

    info->tjmax = TJMAX_DEFAULT;

    if (tcc_read(info->sysfs_path, &info->offset) != 0)
        return -1;

    info->effective = info->tjmax - info->offset;
    return 0;
}

int
tcc_read(const char *path, int *offset)
{
    return read_int(path, offset);
}

int
tcc_write(const char *path, int offset)
{
    return write_int(path, offset);
}

void
tcc_help(void)
{
    printf("Usage: %s [command]\n\n", PROGRAM_NAME);
    printf("Commands:\n");
    printf("  (no args)        Show current TCC offset and effective temp limit\n");
    printf("  set <offset>     Set TCC offset in °C (effective = Tjmax - offset)\n");
    printf("  set-temp <temp>  Set desired max temperature directly (e.g. 85)\n");
    printf("  reset            Reset TCC offset to 0 (full Tjmax)\n");
    printf("  --help, -h       Show this help\n");
    printf("  --version, -v    Show version\n\n");
    printf("Examples:\n");
    printf("  %s                  # shows current status\n", PROGRAM_NAME);
    printf("  %s set 15           # throttle at 85°C (assuming Tjmax = 100°C)\n", PROGRAM_NAME);
    printf("  %s set-temp 85      # same as above, but you type your target temp\n", PROGRAM_NAME);
    printf("  %s reset            # remove the cap, use full Tjmax\n", PROGRAM_NAME);
}

void
tcc_version(void)
{
    printf("%s %s\n", PROGRAM_NAME, VERSION);
}

int
main(int argc, char **argv)
{
    struct tcc_info info;

    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        tcc_help();
        return 0;
    }

    if (argc > 1 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        tcc_version();
        return 0;
    }

    if (tcc_discover(&info) != 0) {
        fprintf(stderr, "error: no TCC offset sysfs interface found.\n");
        fprintf(stderr, "       Your CPU or kernel may not support this feature.\n");
        return 1;
    }

    if (argc == 1) {
        printf("TCC offset:     %d°C\n", info.offset);
        printf("Tjmax:          %d°C\n", info.tjmax);
        printf("Effective max:  %d°C\n", info.effective);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "reset") == 0) {
        if (tcc_write(info.sysfs_path, 0) != 0) {
            fprintf(stderr, "error: failed to write offset (try with sudo?)\n");
            return 1;
        }
        printf("TCC offset reset to 0 — effective max: %d°C\n", info.tjmax);
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "set") == 0) {
        char *end;
        long val = strtol(argv[2], &end, 10);
        if (*end != '\0' || val < 0 || val > info.tjmax) {
            fprintf(stderr, "error: offset must be a number between 0 and %d\n", info.tjmax);
            return 1;
        }
        if (tcc_write(info.sysfs_path, (int)val) != 0) {
            fprintf(stderr, "error: failed to write offset (try with sudo?)\n");
            return 1;
        }
        printf("TCC offset set to %ld°C — effective max: %ld°C\n", val, (long)info.tjmax - val);
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "set-temp") == 0) {
        char *end;
        long val = strtol(argv[2], &end, 10);
        if (*end != '\0' || val < 0 || val > info.tjmax) {
            fprintf(stderr, "error: temperature must be between 0 and %d\n", info.tjmax);
            return 1;
        }
        int offset = info.tjmax - (int)val;
        if (offset < 0) offset = 0;
        if (tcc_write(info.sysfs_path, offset) != 0) {
            fprintf(stderr, "error: failed to write offset (try with sudo?)\n");
            return 1;
        }
        printf("TCC offset set to %d°C — effective max: %ld°C\n", offset, val);
        return 0;
    }

    tcc_help();
    return 1;
}
