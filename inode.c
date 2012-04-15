/*这个是文件系统的核心操作，包括对inode和数据扇区的更改查找等处理*/
#include<stdio.h>
#include<string.h>
#include<assert.h>
#include"struct.h"
#include"global.h"
#include"command_list.h"
#include"inode.h"

#define CAL_POS(sector,size,count)  ((sector*SECTOR_SIZE)+(size)*(count))


/*根据node_num这个inode号来在inode数组中找到对应的具体数据*/
void get_inode(u32 node_num,struct inode *node)
{
    u32 old_pos,pos;
    fgetpos(fimg,&old_pos);
    pos=CAL_POS(spb.inode_array.off_set,sizeof(struct inode),node_num);
    fseek(fimg,pos,SEEK_SET);
    fread(node,sizeof(struct inode),1,fimg);
    fsetpos(fimg,&old_pos);
}

/*根据路径查找对应的inode信息*/
void get_inode_bypath(char *src,struct inode *ret_node)
{
    u32 node_num=pwd,i;
    if(src[0]=='/') node_num=root;
    struct dir_ent ent;
    char *result=NULL,*last=src,*ori_src=src;
    result=strtok(src,"/");
    while(result!=NULL)
    {
        get_inode(node_num,ret_node);
        fseek(fimg,ret_node->i_start_sect*SECTOR_SIZE,SEEK_SET);
        for(i=0;i<ret_node->i_size;i++)
        {
            fread(&ent,sizeof(ent),1,fimg);
            if(strcmp(ent.name,result)==0){
                node_num=ent.i_num;
                break;
            }
        }
        if(i==ret_node->i_size){
            ret_node->i_num=-1;
            return ;
        }
        last=result;
        result=strtok(NULL,"/");
    }
    get_inode(node_num,ret_node);
    if(ori_src!=last)    strcpy(ori_src,last);
}

/*将inode信息写入node_num号对应的inode数组中*/
void write_inode(u32 node_num,struct inode *node)
{
    u32 old_pos,pos;
    fgetpos(fimg,&old_pos);
    pos=CAL_POS(spb.inode_array.off_set,sizeof(struct inode),node_num);
    fseek(fimg,pos,SEEK_SET);
    fwrite(node,sizeof(struct inode),1,fimg);
    fsetpos(fimg,&old_pos);
}

/*给目录增加新的目录项*/
void add_entry(struct inode *cur_dir,struct dir_ent *addent)
{
    u32 old_pos,pos;
    int i,slot;
    struct dir_ent ent;
    fgetpos(fimg,&old_pos);

    pos=CAL_POS(cur_dir->i_start_sect,0,0);
    fseek(fimg,pos,SEEK_SET);
    slot=cur_dir->i_size;
    for(i=0;i<cur_dir->i_size;i++)
    {
        fread(&ent,sizeof(struct dir_ent),1,fimg);
        if(ent.name[0]==0){
            slot=i;
            break;
        }
    }
    if(slot==cur_dir->i_size) cur_dir->i_size++;
    fseek(fimg,pos+sizeof(struct dir_ent)*slot,SEEK_SET);
    fwrite(addent,sizeof(struct dir_ent),1,fimg);
    
    fsetpos(fimg,&old_pos);
}

/*分配inode号，根据inode位图来查找空缺位置，然后填入到inode数组中*/
u32 alloc_inode()
{
    u32 old_pos,retval;
    int i,j;
    unsigned char mask;
    fgetpos(fimg,&old_pos);
    
    fseek(fimg,spb.inode_map.off_set*SECTOR_SIZE,SEEK_SET);
    for(i=0;i<spb.inode_map.size*SECTOR_SIZE;i++)
    {
        fread(&mask,1,1,fimg);
        if(mask!=255){
            for(j=0;j<8;j++)
                if(!(mask&(1<<j))) {
                    mask|=(1<<j);
                     retval=i*8+j;
                    break;
                }
            fseek(fimg,-1,SEEK_CUR);
            fwrite(&mask,1,1,fimg);            
            break;
        }
    }
    fsetpos(fimg,&old_pos);
    return retval;
}
/*释放inode*/
void free_inode(u32 node_num)
{
    u32 old_pos,pos;
    unsigned char mask;
     fgetpos(fimg,&old_pos);

     fseek(fimg,spb.inode_map.off_set*SECTOR_SIZE,SEEK_SET);
     fseek(fimg,node_num/8,SEEK_CUR);
     node_num%=8;
     fread(&mask,1,1,fimg);
     assert(mask &(1<<node_num));
     mask^=(1<<node_num);
     fseek(fimg,-1,SEEK_CUR);
     fwrite(&mask,1,1,fimg);
     
     fsetpos(fimg,&old_pos);
}

/*分配数据扇区，会在数据位图中查找空缺*/
u32 alloc_data()
{
    u32 old_pos,retval;
     int i,j;
     unsigned char mask;
    fgetpos(fimg,&old_pos);

    fseek(fimg,spb.sector_map.off_set*SECTOR_SIZE,SEEK_SET);
    for(i=0;i<spb.sector_map.size*SECTOR_SIZE;i++)
    {
        fread(&mask,1,1,fimg);
        if(mask!=255){
            for(j=0;j<8;j++)
                if(!(mask&(1<<j))){
                    mask|=(1<<j);
                    retval=spb.data_sect+i*8+j;
                    break;
                }
            fseek(fimg,-1,SEEK_CUR);
            fwrite(&mask,1,1,fimg);
            break;
        }
    }
    fsetpos(fimg,&old_pos);
    return retval;
}

/*释放数据扇区*/
void free_sector(u32 sector)
{
    u32 old_pos;
     unsigned char mask;
    fgetpos(fimg,&old_pos);

    sector-=spb.data_sect;    
     fseek(fimg,spb.sector_map.off_set*SECTOR_SIZE,SEEK_SET);
     fseek(fimg,sector/8,SEEK_CUR);
     sector%=8;
     fread(&mask,1,1,fimg);
     assert(mask&(1<<sector));
     mask^=(1<<sector);
     fseek(fimg,-1,SEEK_CUR);
     fwrite(&mask,1,1,fimg);
     
     fsetpos(fimg,&old_pos);
}
