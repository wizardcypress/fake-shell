#include<stdio.h>
#include<string.h>
#include"struct.h"
#include<sys/stat.h>
#include"global.h"
#include"command_list.h"
#include"inode.h"

#define _clear(x)  memset(x,0,sizeof(x))

char src[MAXBUF],dest[MAXBUF],rmfile[MAXBUF];
u32 errno;

/*
  以下为各个处理函数，根据不同的情况都有不同的处理方法，
 但基本上都是基于inode.c中所提供的一些操作函数来对镜像进行读写
*/
void do_ls()
{
    int i;
    struct inode cur_dir,item;
    struct dir_ent ent;
    get_inode(pwd,&cur_dir);
    
    fseek(fimg,cur_dir.i_start_sect*SECTOR_SIZE,SEEK_SET);
    printf("%8s%8s%8s%16s\n","type","inode","size","name");
        //读入各个目录项并显示出来
    for(i=0;i<cur_dir.i_size;i++)
    {
        fread(&ent,sizeof(struct dir_ent),1,fimg);
        if(ent.name[0]=='\0') continue;
        get_inode(ent.i_num,&item);
        printf("%8c%8d%8d%16s\n",item.i_mode==2?'d':'f'
               ,ent.i_num,item.i_size,ent.name);
    }
}

void do_mkdir()
{
    struct inode cur_dir,new_dir;
    char dirname[256];
    struct dir_ent ent;
    scanf("%s",dirname);
    if(strcmp(dirname,".")==0 || strcmp(dirname,"..")==0)
    {
        printf("Error directory name.\n");        
        return ;
    }
    get_inode(pwd,&cur_dir);  
        /*分配一个inode给新的目录*/
    new_dir.i_num=alloc_inode();
    new_dir.i_start_sect=alloc_data();        /*分配一个数据扇区*/
    new_dir.i_nr_sects=1;
    
    new_dir.i_pnum=pwd;
    new_dir.i_mode=2;
    new_dir.i_size=0;

        //给新目录增加默认目录项
    ent.i_num=pwd;    strcpy(ent.name,"..");
    add_entry(&new_dir,&ent);
    ent.i_num=new_dir.i_num;  strcpy(ent.name,".");
    add_entry(&new_dir,&ent);  
 
    ent.i_num=new_dir.i_num;    strcpy(ent.name,dirname);     
    add_entry(&cur_dir,&ent);
    //写入inode数组中
    write_inode(pwd,&cur_dir);
    write_inode(new_dir.i_num,&new_dir); 
}

/*将文件名为 src 的文件复制到 dest 目录下，而且src必须是文件，dest必须是目录 */
void do_cp()
{
    struct inode src_node,dest_node,new_file;
    char buf[MAXBUF];
    if(!src[0]) scanf("%s %s",src,dest);
    get_inode_bypath(src,&src_node); //返回时src只剩下文件名
    get_inode_bypath(dest,&dest_node); 
    if(src_node.i_num==-1 || dest_node.i_num==-1) {
        printf("The path is not correct.\n");
        errno=1;
        _clear(src); _clear(dest);
        return ;
    }
    if(src_node.i_mode==2){
        printf("Source must be a file\n");
        errno=1;
        _clear(src); _clear(dest);
        return ;
    }
    if(dest_node.i_mode==1){
        printf("Destination must be a directory.\n");
        errno=1;
        _clear(src); _clear(dest);
        return ;
    }
    u32 src_sector,dest_sector,last_sector;
    u32 need;    
    struct dir_ent ent;
        //先分配inode给目标文件
    new_file.i_num=alloc_inode();
    new_file.i_pnum=dest_node.i_num;
    new_file.i_size=src_node.i_size;
    new_file.i_mode=1;//文件
    new_file.i_nr_sects=src_node.i_nr_sects;    
    ent.i_num=new_file.i_num;   strcpy(ent.name,src);
        //给目标的目录增加目标文件的目录项
    add_entry(&dest_node,&ent);
    
    need=new_file.i_nr_sects;
    src_sector=src_node.i_start_sect;
    last_sector=0;
        //将src文件逐个扇区复制到 目标文件中
    while(src_sector!=0)
    {
        dest_sector=alloc_data();
        if(last_sector==0)  {
            new_file.i_start_sect=dest_sector;
        }else{
            fseek(fimg,last_sector*SECTOR_SIZE+SECTOR_SIZE-sizeof(u32),SEEK_SET);
            fwrite(&dest_sector,sizeof(u32),1,fimg);            
        }
        fseek(fimg,src_sector*SECTOR_SIZE,SEEK_SET);
        fread(buf,1,SECTOR_SIZE,fimg);
        fseek(fimg,dest_sector*SECTOR_SIZE,SEEK_SET);
        fwrite(buf,1,SECTOR_SIZE,fimg);
        last_sector=dest_sector;

        fseek(fimg,src_sector*SECTOR_SIZE+SECTOR_SIZE-sizeof(u32),SEEK_SET);
        fread(&src_sector,sizeof(u32),1,fimg);
    }
    fseek(fimg,last_sector*SECTOR_SIZE+SECTOR_SIZE-sizeof(u32),SEEK_SET);
    
    write_inode(dest_node.i_num,&dest_node);
    write_inode(new_file.i_num,&new_file);
    _clear(src);  _clear(dest);
    errno=0;
}

void do_mv()
{
    if(!src[0])  scanf("%s %s",src,dest);
    strcpy(rmfile,src);
    errno=0;
    do_cp();
    if(errno==0)     do_rm();
    _clear(src);  _clear(dest);
    _clear(rmfile);
}

/*将 rmfile 这个文件删除，而且目前只能删除文件*/
void do_rm()
{
    struct inode cur_dir,item;
    struct dir_ent ent,emptyent;
    char *filename;
    char buf[MAXBUF];
    memset(&emptyent,0,sizeof(emptyent));
    int i,j;
    if (!rmfile[0]) scanf("%s",rmfile);
    if(!strcmp(rmfile,".") || !strcmp(rmfile,"..")) {
        printf("Can not remove this file.\n");
        _clear(rmfile);
        return ;
    }
    strcpy(buf,rmfile);
    j=-1;
    for(i=0;rmfile[i];i++)   if(rmfile[i]=='/') j=i;
    if(j==0){//   case /dir
        buf[1]='\0';
        filename=rmfile+1;
    }else if(j>0){  //case /dir1/dir2  or ab/cd
        buf[j]='\0';
        filename=rmfile+j+1;
    }else {  //case ab
        strcpy(buf,".");
        filename=rmfile;
    }

    get_inode_bypath(buf,&cur_dir);
    if(cur_dir.i_num==-1) {
        printf("No such file.\n");
        _clear(rmfile);
        return ;
    }
        // get_inode(pwd,&cur_dir);
    fseek(fimg,cur_dir.i_start_sect*SECTOR_SIZE,SEEK_SET);
        //将逐个清空rmfile所占用的扇区
    for(i=0;i<cur_dir.i_size;i++)
    {
        fread(&ent,sizeof(struct dir_ent),1,fimg);
        if(strcmp(filename,ent.name)==0){        
            get_inode(ent.i_num,&item);
            if(item.i_mode==1){
                fseek(fimg,-sizeof(struct dir_ent),SEEK_CUR);
                fwrite(&emptyent,sizeof(struct dir_ent),1,fimg);//删除该目录项,只能删除文件
                free_inode(item.i_num);
                u32 sector=item.i_start_sect;
                while(sector!=0){
                    free_sector(sector);
                    fseek(fimg,sector*SECTOR_SIZE+SECTOR_SIZE-sizeof(u32),SEEK_SET);
                    fread(&sector,sizeof(u32),1,fimg);
                }
            }else printf("Can not remove directory\n");
            _clear(rmfile);
            return ;
        }
    }
    _clear(rmfile);
    printf("No such file.\n");
}

void do_cd()
{
    char dirname[256];
    struct inode cur_dir,new_dir;
    struct dir_ent ent;
    int i,j,last;
    scanf("%s",dirname);
    if(strcmp(dirname,".")==0) return ;
    get_inode(pwd,&cur_dir);
    fseek(fimg,cur_dir.i_start_sect*SECTOR_SIZE,SEEK_SET);
    for(i=0;i<cur_dir.i_size;i++)
    {
        fread(&ent,sizeof(struct dir_ent),1,fimg);
        if(strcmp(ent.name,dirname)==0){
                //只能cd 目录
            get_inode(ent.i_num,&new_dir);
            if(new_dir.i_mode==1){
                printf("Can not cd a file.\n");
                return ;
            }
            if(strcmp(dirname,"..")==0){
                if(pwd==root) return ;
                last=0;
                for(j=0;path[j];j++) if(path[j]=='/') last=j;
                if(last==0) last++;
                path[last]='\0';
                pwd=ent.i_num;
            }else{
                if(strcmp(path,"/")) strcat(path,"/");
                strcat(path,ent.name);
                pwd=ent.i_num;
            }
            return ;
        }
    }
    printf("No such file or directory.\n");
}

/* 将外部文件写入到镜像中 */
void do_write()
{
    struct stat st;
    struct inode cur_dir,new_file;
    struct dir_ent ent;
    char buf[SECTOR_SIZE+10];
    int i;
    FILE *fin;
    char filename[16];
    scanf("%s",filename);
    if(stat(filename,&st)<0) {
        printf("No such file.\n");
        return ;
    }
    fin=fopen(filename,"rb");
    get_inode(pwd,&cur_dir);
    new_file.i_num=alloc_inode();
    new_file.i_pnum=pwd;
    new_file.i_size=st.st_size; 
    new_file.i_mode=1; //标示为文件
    ent.i_num=new_file.i_num;   strcpy(ent.name,filename);    
    add_entry(&cur_dir,&ent);
    
    u32 last_sector=0,cur_sector;
    u32 need=st.st_size/(SECTOR_SIZE-sizeof(u32))+1;
    new_file.i_nr_sects=need;
        //根据文件大小，逐个分配扇区来写入数据,并采用单链表方式连接各个扇区
    for(i=0;i<need;i++)
    {
        cur_sector=alloc_data();
        if(last_sector==0)  {
            new_file.i_start_sect=cur_sector;
        }else{ //上一个扇区到这个扇区的指针
            fseek(fimg,last_sector*SECTOR_SIZE+SECTOR_SIZE-sizeof(u32),SEEK_SET);
            fwrite(&cur_sector,sizeof(u32),1,fimg);
        }
        int nread=fread(buf,sizeof(char),SECTOR_SIZE-sizeof(u32),fin);
        if(nread<0)  {
            printf("Read file %s error.\n",filename );
            return ;
        }
        fseek(fimg,cur_sector*SECTOR_SIZE,SEEK_SET);
        fwrite(buf,sizeof(char),nread,fimg);
        last_sector=cur_sector;
    }
    fseek(fimg,last_sector*SECTOR_SIZE+SECTOR_SIZE-sizeof(u32),SEEK_SET);
    u32 tmp=0;
    fwrite(&tmp,sizeof(u32),1,fimg);
    write_inode(new_file.i_num,&new_file);
    write_inode(cur_dir.i_num,&cur_dir);
}
