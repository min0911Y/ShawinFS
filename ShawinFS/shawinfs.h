#pragma once
#include <stdint.h>
#include <stdbool.h>
#define __packed 

// ShawinFS的数据结构
#ifdef _MSC_VER
#pragma pack(1)
#endif // _MSC_VER
typedef struct {
    char identify[5]; // 标记文件系统名
    char block_size; // 块大小，单位是K（如果为1，块大小为1K，以此类推）
    int block_start; // 标记块的起始位
    int block_total; // 标记可使用块的数量 （不包括位图和块索引）
    int bitmap_start; // 位图的起始位置
    int bitmap_size; // 位图占用Block数
    int block_index_start; // 块索引起始
    int block_index_size; // 块索引大小
    int block_resd_start; // 保留块起始，不保留填0
    int block_resd_end; // 保留块结束，不保留填0
    int root_block; // 指示根目录inode块
    char volid[5]; // 卷名
} __packed shawinfs_mbr_t;

typedef enum {
    SHAWIN_NULL = 0,SHAWIN_FILE, SHAWIN_DIR, SHAWIN_LFN
} shawinfs_types_t;

typedef struct {
    uint8_t r : 1; // read
    uint8_t w : 1; // write
    uint8_t x : 1; // execute
    uint8_t resd : 5; // 保留
} __packed shawinfs_inode_attr_t;
typedef struct {
    shawinfs_types_t type; // 描述该inode类型
    char name[13]; // 这个inode的名称
    shawinfs_inode_attr_t attr; // 属性
    uint32_t block_index_id; // 块索引信息
    uint32_t next; // 接下来的名字（要支持长文件名），如果这个项被值位，那么就说明文件名还没结束 （接下来文件名在inode的第几项）
} __packed shawinfs_inode_file_t;

typedef struct {
    shawinfs_types_t type; // 描述该inode类型
    char name[18]; // 这个inode的名称（接上之前的）
    uint32_t next; // 接下来的名字（要支持长文件名），如果这个项被值位，那么就说明文件名还没结束 （接下来文件名在inode的第几项）
} __packed shawinfs_inode_lfn_t;
#ifdef _MSC_VER
#pragma pack(0)
#endif // _MSC_VER

#define SHAWINFS_MAGIC "\xffSWFS"
#define SHAWINFS_MAGIC_SIZE 5

// 下面结构体用来寄存一些东西

typedef struct {
    uint32_t disk_number; // 需要读取的磁盘号
    shawinfs_mbr_t mbr; // 需要mbr中的一些信息
    // bitmap 缓存，这样不用每次读取都重新read一遍
    uint32_t bitmap_cache_size;
    uint8_t* bitmap_cache;
    // block_index缓存
    uint32_t block_index_cache_size;
    uint8_t* block_index_cache;
    uint32_t size_per_sector;
} shawinfs_t; // 存储一些必要信息


bool shawinfs_format(int disk_number, int block_resd_start, int block_resd_end,
    int block_size);