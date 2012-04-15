#ifndef _STRUCT_H
#define _STRUCT_H

typedef unsigned int u32;

struct segment{
    u32 off_set;
    u32 size;
};
struct inode{
    u32 i_num;  //  inode 号
    u32 i_pnum;  //父亲inode 号
    u32 i_size;  //如果是文件,i_size表示文件大小,如果是目录，则表示有多少个目录项
    u32 i_start_sect; //该文件/目录的开始扇区
    u32 i_nr_sects;//占用的扇区
    u32 i_mode;  // 1=file  2=directory
};
struct super_block{ //文件系统的超级块
    u32 magic;   //magic号，用于标示文件系统ID
    struct segment inode_map;;  //inode的位图表
    struct segment inode_array;    //inode数组位置，用于记录每个inode的具体信息。
    struct segment sector_map; //扇区的位图表，从数据区开始算起    
    u32 data_sect;//数据区的开始扇区
};

struct dir_ent{  //目录项
    u32 i_num;   //该项的inode号
    char name[12]; //该项的名字
};

struct command{ 
    char cmd[256];
    void (*fptr)();    
};

#endif
