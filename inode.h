#ifndef _INODE_H
#define _INODE_H

 void get_inode(u32 node_num,struct inode *node);
 void write_inode(u32 node_num,struct inode *node);
 void write_entry(u32 node_num,u32 entry_num,struct dir_ent *ent);
u32 alloc_inode();
u32 alloc_data();
void add_entry(struct inode *cur_dir,struct dir_ent *addent);
void free_inode(u32 node_num);
void free_sector(u32 sector);
void get_inode_bypath(char *src,struct inode *src_node); //返回时src只剩下文件名

#endif
