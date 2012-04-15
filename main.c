#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"struct.h"
#include"global.h"
#include"command_list.h"
#include"inode.h"

struct command command_list[]={  //各个命令名字和所对应的处理函数
    {"ls",do_ls},
    {"mkdir",do_mkdir},
    {"mv",do_mv},
    {"cp",do_cp},
    {"rm",do_rm},
    {"cd",do_cd},
    {"write",do_write},
    {0,0}
};

//全局的数据结构
u32 pwd,root; //pwd表示当前目录的inode号，root是根目录的inode号
FILE *fimg;  //镜像文件
char path[1024]; //用于表示当前路径


struct super_block spb={ //superblock的信息，记录了整个文件系统相应部分的情况
    0x1111,
    {2,1},
    {3,4096*sizeof(struct inode)/SECTOR_SIZE},
    {4+4096*sizeof(struct inode)/SECTOR_SIZE,5},
    9+4096*sizeof(struct inode)/SECTOR_SIZE
};

void makefs()  //初始化文件系统
{    
     int i=1;
     char ch='\0';
     struct dir_ent root_ent;
     struct inode root_inode;
     fimg=fopen("a.img","rb+");
     for(i=0;i<DISK_SIZE;i++) fwrite(&ch,1,1,fimg);
        //把superblock写入文件系统对应位置
    fseek(fimg,SECTOR_SIZE,SEEK_SET);
    fwrite(&spb,sizeof(struct  super_block),1,fimg);
    
    root_inode.i_num=alloc_inode();
    root_inode.i_start_sect=alloc_data();    
    root_inode.i_nr_sects=1;
    root_inode.i_pnum=root_inode.i_num;
    root_inode.i_size=0;
    root_inode.i_mode=2;
        //加入默认的目录项
    root_ent.i_num=root;     strcpy(root_ent.name,".");
    add_entry(&root_inode,&root_ent);
    strcpy(root_ent.name,"..");
    add_entry(&root_inode,&root_ent);
        
    write_inode(root_inode.i_num,&root_inode);
    
    pwd=root=root_inode.i_num;
    strcpy(path,"/");
    fclose(fimg);
}
void handle_command()  //逐个读入命令，并用相应的函数去处理
{
    char cmd[256];
    int i;
    fimg=fopen("a.img","rb+");
    printf("%s $ ",path);
    while(scanf("%s",cmd)!=EOF)
    {
        for( i=0;command_list[i].cmd[0];i++)
            if(strcmp(command_list[i].cmd,cmd)==0){
                (command_list[i].fptr)();
                break;
            }
        if(!command_list[i].cmd[0])
            printf("No such command.\n");
       printf("%s $ ",path);
    }
}
int main()
{
    makefs();
    handle_command();
    return 0;
}
