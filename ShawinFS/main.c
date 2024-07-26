#define _CRT_SECURE_NO_WARNINGS
#include "shawinfs.h"
#include <stdio.h>
#include <errno.h>
FILE* fp;
void shawinfs_hal_read(uint32_t disk_number, uint8_t *buffer, uint32_t lba,
                       uint32_t numb) {
    fseek(fp, lba * 512, SEEK_SET);
    fread(buffer, 1, numb * 512, fp);
}
void shawinfs_hal_write(uint32_t disk_number, uint8_t *buffer, uint32_t lba,
                        uint32_t numb) {
    fseek(fp, lba * 512, SEEK_SET);
    fwrite(buffer, 1, numb * 512, fp);
}
uint32_t shawinfs_hal_get_size(uint32_t disk_number) {
    fseek(fp, 0, SEEK_END);
    long val = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return val;
}

int main() {
    fp = fopen("disk.img", "r+");
    if (fp == NULL) {
        printf("can't fopen\n");
        return 1;
    }
    shawinfs_format(0, 0, 0, 4096);

    fclose(fp);
}