#include "server.h"

char _cwd[BUF_LEN] = "";
char *currentdir()   //获取并返回当前工作目录的路径
{
    if ((getcwd(_cwd, BUF_LEN)) == NULL)
        exit_perror("getcwd");
    return _cwd;
}

struct func_table ftab[8] = {
    {QUIT, quit},
    {PWD, pwd},
    {CWD, cwd},
    {LIST, list},
    {RETR, retr},
    {STOR, stor},
    {RM, rm},
//    {RM,rm},
    {-1, NULL}};

void quit(int sock, struct ftph *buf)
{
    ssend(sock, OK, CODE0, "");
    close(sock);
    printf("closed\n");
    exit(0);
}


void pwd(int sock, struct ftph *buf)  //获取当前工作目录
{
    ssend(sock, OK, CODE0, currentdir());
}


void cwd(int sock, struct ftph *buf)
{
    // change current dir

    printf("> change current dir: %s\n", buf->data);

    if (chdir(buf->data) < 0)
    {
        perror("chdir");
        init_ftph(buf);
        if (errno == EACCES)
        {
            ssend(sock, FILE_ERR, CODE1, "");
        }
        else if (errno == ENOENT || errno == ENOTDIR)
        {
            ssend(sock, FILE_ERR, CODE0, "");
        }
        else
        {
            ssend(sock, FILE_ERR, CODE5, "");
        }
        return;
    }

    ssend(sock, OK, CODE0, "");
}

void list(int sock, struct ftph *buf)
{
    struct stat st;
    if (ntohs(buf->length) > 0 && stat(buf->data, &st) != 0)
    {
        perror("stat");
        ssend(sock, FILE_ERR, CODE0, "");
        return;
    }

    FILE *fp;
    char cmd[BUF_LEN] = "ls -l ";
    strcat(cmd, buf->data);
    // ack or error
    if (ntohs(buf->length) > 0 && !S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))
    {
        ssend(sock, FILE_ERR, CODE0, "");
        return;
    }
    else if ((fp = popen(cmd, "r")) == NULL)
    {
        perror("popen");
        ssend(sock, FILE_ERR, CODE0, "");
        close(fp);
        return;
    }

    // ack
    ssend(sock, OK, CODE1, "");

    char str[BUF_LEN][BUF_LEN];
    int code = CODE0, i = 0, n = 0;
    while (!feof(fp))
    {
        memset(str[i], 0, sizeof(char) * BUF_LEN);
        fgets(str[i], sizeof(char) * BUF_LEN, fp);
        if (strlen(str[i]) > 0)
        {
            str[i][strlen(str[i]) - 1] = '\0';
            i++;
        }
    }
    while (n < i)
    {
        code = n == i - 1 ? CODE0 : CODE1;
        printf("> %s\n",str[n]);
        ssend(sock, DATA, code, str[n++]);
    }

    close(fp);
}

void retr(int sock, struct ftph *buf)  //用于从服务器检索（下载）文件
{
    int n, fd;
    char *filename = buf->data;

    struct stat st;
    if (stat(filename, &st) != 0)
    {
        perror("stat");
        ssend(sock, FILE_ERR, CODE0, "");
        return;
    }

    if (S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))
    {
        ssend(sock, FILE_ERR, CODE0, "");
        return;
    }
    else if ((fd = open(filename, O_RDONLY)) < 0)
    {
        if (errno == EPERM || errno == EACCES)
        {
            fprintf(stderr, "no permittion\n");
            ssend(sock, FILE_ERR, CODE1, "");
            return;
        }
        fprintf(stderr, "not found: %s\n", filename);
        ssend(sock, FILE_ERR, CODE0, "");
        return;
    }

    printf("> read %s ...\n", filename);
    printf("> send ack\n"), fflush(stdout);

    // ok ack
    ssend(sock, OK, CODE1, "");

    // send file
    printf("> send file\n"), fflush(stdout);
    int code = CODE0;
    char str[BUF_LEN];
    while ((n = read(fd, str, sizeof(char) * BUF_LEN)) >= 0)
    {
        printf("> reading %d bytes\n", n), fflush(stdout);
        code = n < sizeof(char) * BUF_LEN ? CODE0 : CODE1;
        _ssend(sock, DATA, code, str, n);
        memset(str, 0, sizeof(char) * BUF_LEN);
        if (code == CODE0)
            break;
    }

    printf("> ok\n"), fflush(stdout);
    close(fd);
}

void stor(int sock, struct ftph *buf)   //用于向服务器存储（上传）文件
{
    char *filename;
    mem_alloc(filename, char, strlen(buf->data), EXIT_FAILURE);
    strcpy(filename, buf->data);

    int fd, n;
    printf("> ack => ok\n"), fflush(stdout);
    if ((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0755)) < 0)
    {
        perror("open");
        if (errno == EPERM || errno == EACCES)
            ssend(sock, FILE_ERR, CODE1, "");

        free(filename);
        return;
    }

    printf("> store %s ...\n", filename);
    ssend(sock, OK, CODE2, ""); // ack

    // get data
    printf("> downloading %s ... ", filename), fflush(stdout);
    init_ftph(buf);
    buf->type = DATA, buf->code = CODE1;
    while (cont_data(buf))
    {
        free(buf);
        buf = sread(sock);
        if (cont_data(buf) || nocont_data(buf))
            write(fd, buf->data, sizeof(char) * ntohs(buf->length));
    }

    // end-check
    if (!nocont_data(buf))
    {
        fprintf(stderr, "unknown or invalid response\n");
        print_header(buf);
        return;
    }

    printf("fin\n");
    close(fd);
    free(filename);
}

void rm(int sock, struct ftph *buf) {
    // 检查buf和buf->data是否为NULL
    if (buf == NULL || buf->data == NULL) {
        ssend(sock, FILE_ERR, CODE1, "Invalid request");
        return;
    }

    char *path = buf->data;  // 从buf中获取要删除的路径

    // 检查路径长度
    if (strlen(path) >= MAX_STR_LEN) {
        ssend(sock, FILE_ERR, CODE1, "Path too long");
        return;
    }

    // 尝试删除文件或目录
    if (remove(path) == 0) {
        // 删除成功
        ssend(sock, OK, CODE0, "File removed successfully");
    } else {
        // 删除失败，可能是因为文件不存在或没有权限
        perror("remove");
        ssend(sock, FILE_ERR, CODE1, "Failed to remove file");
    }
}



//
////用于删除文件
//void rm(int sock, struct ftph *buf)
//{
//    char path[BUF_LEN];
//    // Assuming buf->arg contains the path
//    strncpy(path, buf->arg, BUF_LEN);
//
//    // Perform the deletion operation
//    if (remove_directory_or_file(path) == 0)
//    {
//        ssend(sock, OK, CODE0, "File/Directory removed successfully");
//    }
//    else
//    {
//        ssend(sock, ERR, CODE1, "Error removing file/directory");
//    }
//}
//
//// Function to recursively delete files and directories
//int remove_directory_or_file(const char *path)
//{
//    struct stat path_stat;
//    stat(path, &path_stat);
//
//    if (S_ISDIR(path_stat.st_mode)) {
//        // It's a directory, perform recursive deletion
//        DIR *d = opendir(path);
//        if (d) {
//            struct dirent *p;
//            while ((p = readdir(d)) != NULL) {
//                if (strcmp(p->d_name, ".") != 0 && strcmp(p->d_name, "..") != 0) {
//                    char full_path[BUF_LEN];
//                    snprintf(full_path, BUF_LEN, "%s/%s", path, p->d_name);
//                    remove_directory_or_file(full_path);
//                }
//            }
//            closedir(d);
//            return rmdir(path);
//        }
//    } else {
//        // It's a file
//        return remove(path);
//    }
//    return -1; // Error case
//}
