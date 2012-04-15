
//全局的数据,主要定义了文件系统的大小和扇区大小
#ifndef _GLOBAL_H
#define _GLOBAL_H

#define MAX_NAME 256
#define MAX_INODE 4096
#define MAXBUF 1024
#define DISK_SIZE 1024*1024*10
#define SECTOR_SIZE 512

extern u32 pwd,root;
extern struct super_block spb;
extern FILE *fimg;
extern char path[MAXBUF];
extern char src[MAXBUF],dest[MAXBUF],rmfile[MAXBUF];

#endif
