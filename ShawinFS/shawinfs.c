// ShawinFS实现
// Copyright (C) 2024 min0911 & Rainy101112
// GPL3.0 开源
//----------------------------------------------------------------------
#include "shawinfs.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// ShawinFS抽象层，读写存储器
// 须自行实现
// HARD-CODE ！！！警告：扇区大小写死为512字节！！！
//---------------------------HAL----------------------------------------
void shawinfs_hal_read(uint32_t disk_number, uint8_t *buffer, uint32_t lba,
                       uint32_t numb);
void shawinfs_hal_write(uint32_t disk_number, uint8_t *buffer, uint32_t lba,
                        uint32_t numb);
uint32_t shawinfs_hal_get_size(uint32_t disk_number);
//----------------------Marco definitions-------------------------------

// n是1-4的整数，用于从mbr的BLOCK_SIZE参数中获取真正的BLOCK_SIZE数值
#define _GET_BLOCK_SIZE(n) (n * 1024)
// r/w block
#define _shawinfs_read_block()
#define _shawinfs_write_block
// 位操作 dat是1个字节，n的取值为0~7
// SW_DETECT_BIT : 如果dat的第n位为1，返回一个非零值，如果是0，则返回0
// SW_SET_BIT : 设置dat中的第n位
// SW_CLEAR_BIT : 清除dat中的第n位
#define SW_DETECT_BIT(dat, n) ((dat) & (1 << n))
#define SW_SET_BIT(dat, n) dat = ((dat) | (1 << n))
#define SW_CLEAR_BIT(dat, n) dat = ((dat) & ~(1 << n))
//----------------------------------------------------------------------

/// <summary>
/// 获得 被除数/除数的值
/// 如果不能整除，则向上加1
/// </summary>
/// <param name="num">被除数</param>
/// <param name="size">除数</param>
/// <returns>商</returns>
static unsigned shawinfs_div_round_up(unsigned num, unsigned size) {
  return (num + size - 1) / size;
}

/// <summary>
/// 初始化shawinfs实例
/// </summary>
/// <param name="disk_number">磁盘号</param>
/// <returns>shawinfs实例</returns>
shawinfs_t *shawinfs_init(uint32_t disk_number) {
    shawinfs_t* swfs = (shawinfs_t*)malloc(sizeof(shawinfs_t));
}
/// <summary>
/// 检查是否是shawinfs
/// </summary>
/// <param name="disk_number">磁盘号</param>
/// <param
/// name="size_per_sector">扇区大小（单位：字节）（实际上你也只能填写512）</param>
/// <returns>true 是shawinFS false: 不是</returns>
bool shawinfs_check(
    uint32_t disk_number,
    uint32_t size_per_sector) { // 这里看似可以自定义扇区大小，实际上只能填写512
  uint8_t *mbr = (uint8_t *)malloc(size_per_sector); // 分配一个内存读取mbr
  assert(mbr);
  shawinfs_hal_read(disk_number, mbr, 0, 1);
  shawinfs_mbr_t *shawinfs_mbr = (shawinfs_mbr_t *)mbr;
  bool result = memcmp(shawinfs_mbr->identify, SHAWINFS_MAGIC,
                       SHAWINFS_MAGIC_SIZE) == 0; // 比较标识符
  free(mbr);
  return result;
}
/// <summary>
/// 读取一个block
/// </summary>
/// <param name="disk_number">磁盘号</param>
/// <param name="block_start">block开始扇区</param>
/// <param name="block_size">一个block的大小</param>
/// <param name="block_number">block号</param>
/// <param name="count">读几个block</param>
/// <param name="buff">缓冲区</param>
static void __shawinfs_read_block(uint32_t disk_number, uint32_t block_start,
                                  uint32_t block_size, uint32_t block_number,
                                  uint32_t count, uint8_t *buff) {
  uint32_t sector_need_to_read =
      block_size * count / 0x200; // 我们默认扇区512字节
  uint32_t lba = block_number * block_size / 0x200 + block_start;
  shawinfs_hal_read(disk_number, buff, lba, sector_need_to_read);
}
/// <summary>
///  写一个block
/// </summary>
/// <param name="disk_number">磁盘号</param>
/// <param name="block_start">block开始扇区</param>
/// <param name="block_size">一个block的大小</param>
/// <param name="block_number">block号</param>
/// <param name="count">读几个block</param>
/// <param name="buff">缓冲区</param>
static void __shawinfs_write_block(uint32_t disk_number, uint32_t block_start,
                                   uint32_t block_size, uint32_t block_number,
                                   uint32_t count, uint8_t *buff) {
  uint32_t sector_need_to_read =
      block_size * count / 0x200; // 我们默认扇区512字节
  uint32_t lba = block_number * block_size / 0x200 + block_start;
  shawinfs_hal_write(disk_number, buff, lba, sector_need_to_read);
}
/// <summary>
///  在bitmap中设置某个block已经分配
/// </summary>
/// <param name="buffer">bitmap缓冲区</param>
/// <param name="start">起始</param>
/// <param name="end">结束</param>
static void __shawinfs_set_bitmap_buffer_alloced(uint8_t *buffer,
                                                 uint32_t start, uint32_t end) {
  uint32_t idx = start / 8; // 计算我们要操作的起始字节在buffer中的位置
  uint32_t count = end - start; // 要操作这么多个嘛
  uint8_t n = start % 8;        // 记录一下当前在操作第几个字符
  for (int i = 0; i < count; i++) {
    if (n == 8) { // 超过1个字节了
      n = 0;      // 归零
      idx++;      // 我们现在操作下一个字节
    }
    // 设置
    SW_SET_BIT(buffer[idx], n);
    n++; // 下一个bit
  }
}
/// <summary>
///  在bitmap中设置某个block空闲
/// </summary>
/// <param name="buffer">bitmap缓冲区</param>
/// <param name="start">起始</param>
/// <param name="end">结束</param>
static void __shawinfs_set_bitmap_buffer_free(uint8_t *buffer, uint32_t start,
                                              uint32_t end) {
  uint32_t idx = start / 8; // 计算我们要操作的起始字节在buffer中的位置
  uint32_t count = end - start; // 要操作这么多个嘛
  uint8_t n = start % 8;        // 记录一下当前在操作第几个字符
  for (int i = 0; i < count; i++) {
    if (n == 8) { // 超过1个字节了
      n = 0;      // 归零
      idx++;      // 我们现在操作下一个字节
    }
    // 清除
    SW_CLEAR_BIT(buffer[idx], n);
    n++; // 下一个bit
  }
}

/// <summary>
/// 将磁盘格式化为shawinfs
/// 注意：如果不保留块，BlockResdStart和BlockResdEnd都应为0。如果保留，
/// BlockResdEnd需要在有数据的块号后面加1。例如保留3 4 5号块，
/// BlockResdStart=3，BlockResdEnd=6，以及，这两个的单位是扇区，
/// 且代表的不是BLOCK号，而是物理扇区号
/// </summary>
/// <param name="disk_number">磁盘号</param>
/// <param name="block_resd_start">保留扇区的起始地址（不保留填写0）</param>
/// <param name="block_resd_end">保留扇区的结束地址（不保留填写0）</param>
/// <param name="block_size">Block的大小（只能为1K 2K 4K）</param>
/// <returns>是否成功（成功true 失败false）</returns>
bool shawinfs_format(int disk_number, int block_resd_start, int block_resd_end,
                     int block_size) {
  // 块大小只能为  1K、2K、4K
  if (block_size != 1024 && block_size != 2048 && block_size != 4096) {
    return false;
  }
  /* 处理MBR */

  // 之后这里改为读取一个文件 （如boot.bin）
  uint8_t buf[0x200] = { 0 };
  shawinfs_mbr_t *mbr = buf;
  // 设置标识符
  memcpy(mbr->identify, SHAWINFS_MAGIC, SHAWINFS_MAGIC_SIZE);
  mbr->block_size = block_size / 1024; // 设置块大小
  if (block_resd_end)
    mbr->block_start = block_resd_end;
  else {
    mbr->block_start = 1; // 第零扇区的是mbr
  }
  uint32_t total_sectors =
      shawinfs_hal_get_size(disk_number) / 0x200; // 一个扇区的大小是0x200字节

  //// 算出除数：
  // 我们要知道余下的这么多扇区，到底是多少个BLOCK，所以我们需要计算这个除数
  // 这就是为什么我需要把BLOCK大小限制在1K 2K 4K的原因
  uint32_t div = _GET_BLOCK_SIZE(mbr->block_size) / 0x200;
  mbr->block_total = (total_sectors - mbr->block_start) / div;
  mbr->bitmap_start = 0; // BLOCK值
  // bitmap是位图，一字节可以存储8个block的信息
  mbr->bitmap_size = shawinfs_div_round_up(
      shawinfs_div_round_up(mbr->block_total, 8), /* 我们需要向上取整 */
      _GET_BLOCK_SIZE(mbr->block_size));                           /* 同理 */
  /* 最耗空间的一个东西 */
  mbr->block_index_start =
      mbr->bitmap_start + mbr->bitmap_size; // 我们紧紧跟在后面即可
  mbr->block_index_size =
      shawinfs_div_round_up(mbr->block_total, 4096); // 需要占用这么多
  mbr->root_block =
      mbr->block_index_start + mbr->block_index_size; // 也一样跟在后面
  memcpy(mbr->volid, "SHWFS", 5);
  // 接下来写入mbr
  shawinfs_hal_write(disk_number, buf, 0, 1);

  // 初始化bitmap，我们先全部置为0
  uint8_t *temp_bitmap = (uint8_t *)malloc(_GET_BLOCK_SIZE(mbr->block_size)); // 空的bitmap
  assert(temp_bitmap);
  memset(temp_bitmap, 0, _GET_BLOCK_SIZE(mbr->block_size));
  for (int i = 0; i < mbr->bitmap_size; i++) {
    __shawinfs_write_block(disk_number, mbr->block_start, _GET_BLOCK_SIZE(mbr->block_size),
                           mbr->bitmap_start + i, 1, temp_bitmap);
  }

  // 接下来设置bitmap：
  // 设置bitmap所占用的bitmap占用以及block_index_size占用，因为是连起来的，所以直接加上去就好
  // 剩下那个1是root_block
  int current_block = mbr->bitmap_start;
  for (int i = mbr->bitmap_size + mbr->block_index_size + 1; i;) {
    // 1个字节可以表示八个block
    // 那么1个block能够标记的block数量就是block_size * 8
    // 如果一次性可以结束，那就一次性结束，否则就多弄几次
    int c =
        i > _GET_BLOCK_SIZE(mbr->block_size) * 8 ? _GET_BLOCK_SIZE(mbr->block_size) * 8 : i; // 这一次我们要设置的
    __shawinfs_set_bitmap_buffer_alloced(temp_bitmap, 0, c);
    // 写盘
    __shawinfs_write_block(disk_number, mbr->block_start, _GET_BLOCK_SIZE(mbr->block_size),
                           current_block++, 1, temp_bitmap);
    i -= c;
    memset(temp_bitmap, 0, _GET_BLOCK_SIZE(mbr->block_size));
  }
  free(temp_bitmap);
}