#ifndef TEMPCAP_H
#define TEMPCAP_H

#define PROGRAM_NAME "tempcap"
#define VERSION      "1.0.0"

#define TJMAX_DEFAULT 100
#define SYSFS_PATH_MAX 4096

struct tcc_info {
    char sysfs_path[SYSFS_PATH_MAX];
    int  offset;
    int  tjmax;
    int  effective;
};

int  tcc_discover(struct tcc_info *info);
int  tcc_read(const char *path, int *offset);
int  tcc_write(const char *path, int offset);
void tcc_help(void);
void tcc_version(void);

#endif
