#include "myftp.h"

struct func_table
{
    int type;
    void (*func)(int sock, struct ftph *buf);
};

extern struct func_table ftab[8];

char *currentdir();
void quit(int sock, struct ftph *buf),
    pwd(int sock, struct ftph *buf),
    cwd(int sock, struct ftph *buf),
    list(int sock, struct ftph *buf),
    retr(int sock, struct ftph *buf),
    stor(int sock, struct ftph *buf),
            rm(int sock, struct ftph *buf);

//    rm(int sock, struct ftph *buf);

//int remove_directory_or_file(const char *path);
