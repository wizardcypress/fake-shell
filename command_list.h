
#ifndef _COMM_LIST_H
#define _COMM_LIST_H

typedef struct{
    char cmd[256];
    void (*fptr)();
}command;

 void do_ls();
 void do_mkdir();
void do_cp();
void do_mv();
void do_rm();
 void do_cd();
void do_write();

#endif
