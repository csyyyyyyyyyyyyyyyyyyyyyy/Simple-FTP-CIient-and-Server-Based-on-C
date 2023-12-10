#include "server.h"
#include <sqlite3.h>
#include <openssl/sha.h>

//int authenticate(const char *username, const char *password);
int authenticate_or_register(const char *username, const char *password, int is_register);
int add_user(const char *username, const char *password);
int user_exists(const char *username);
void sha256_hash_string(unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65]);
sqlite3 * db;


int main(int argc, char **argv)
{

    if (argc > 2)
        exit_fprintf_("Usage: %s [current-dir]\n", argv[0]);

    int alen;   //size of client address
    int sock0, sock;  //sock0:server socket, sock:client socket
    struct sockaddr_in saddress, caddress;  //用于存储服务器和客户端的地址信息

    struct func_table *ft;
    struct ftph * buf;
    int tmp = 1;
    mem_alloc(buf, struct ftph, 1, EXIT_FAILURE);


    if (argc == 2)  //这个块检查是否有第二个命令行参数。如果有，程序会改变当前工作目录到该参数指定的目录
    {
        printf("Change current dir: %s\n", argv[1]);
        if (chdir(argv[1]) < 0)
            exit_perror("chdir");
    }
    printf("Prepare socket ...\n");

    // open socket
    if ((sock0 = socket(AF_INET, SOCK_STREAM, 0)) < 0)  //创建一个IPv4的TCP套接字。如果创建失败，程序将退出
        exit_perror("socket");


    //设置套接字选项，允许重新使用本地地址和端口
    if (setsockopt(sock0, SOL_SOCKET, SO_REUSEADDR, (const char *)&tmp, sizeof(tmp)) < 0)
        exit_perror("setsockopt");

    // setup ip address and port
    memset(&saddress, 0, sizeof(saddress));
    saddress.sin_family = AF_INET;
    saddress.sin_port = htons(TCP_PORT);
    saddress.sin_addr.s_addr = htonl(INADDR_ANY);


    // bind
    if (bind(sock0, (struct sockaddr *)&saddress, sizeof(saddress)) < 0)
        exit_perror("bind");

    // listen
    if (listen(sock0, MAX_LISTEN) != 0)
        exit_perror("listen");

    int isparent = 1;



    while (1)
    {
        if (isparent > 0)
        {
            printf("[parent] Waiting connection...\n");
            alen = sizeof(caddress);
            if ((sock = accept(sock0, (struct sockaddr *)&caddress, (socklen_t *)&alen)) < 0)
                exit_perror("accept");


            //Open database
            if (sqlite3_open("myftpdb.sqlite", &db) != SQLITE_OK) {
                fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);
                return 1;
            }

            //增添authentication部分
            send(sock, "Please authenticate\n",20,0); //发送提示信息,20为发送的字节数

            int is_register;
            char username[100]; // Adjust size as needed
            char password[100]; // Adjust size as needed
            recv(sock, &is_register, sizeof(is_register), 0); //接收注册信息(0为登录，1为注册
            recv(sock, username, 100, 0); //接收用户名
            recv(sock, password, 100, 0); //接收密码

            // Verify credentials
            // Check if it's a registration attempt
            if(is_register) {
                // Check if user already exists and add the user if not
                if (!user_exists(username)) {
                    if (add_user(username, password)) {
                        printf("New user added successfully.\n");
                        send(sock, "Registration successful\n", 24, 0);
                    } else {
                        printf("Failed to add new user.\n");
                        send(sock, "Registration failed\n", 20, 0);
                    }
                } else {
                    send(sock, "User already exists\n", 21, 0);
                }
            } else {
                // Proceed with authentication
                if (authenticate_or_register(username, password,is_register)) {
                    send(sock, "Authentication successful\n", 27, 0);
                    // Proceed with normal operation
                } else {
                    send(sock, "Authentication failed\n", 23, 0);
                    close(sock);
                }
            }

            sqlite3_close(db);



            if ((isparent = fork()) < 0) // fork
            {
                exit_perror("fork");
            }
            if (isparent)
                printf("> connected!\n> forked child process\n");
        }

        if (!isparent) // child
        {
            printf(">> current dir: %s <<\n", currentdir());

            if ((buf = sread(sock)) == NULL)
            {
                printf("[child] disconnected\n");
                close(sock);
                free(buf);
                exit(0);
            }

            printf(">> Receive Message << \n"), print_header(buf);
            for (ft = ftab; ft->func != NULL; ft++)
            {
                if (ft->type == buf->type)
                {
                    (ft->func)(sock, buf);
                    break;
                }
            }
        }
        else // parent
        {
        }
    }

    close(sock0);
    free(buf);

    return 0;
}


int authenticate_or_register(const char *username, const char *password, int is_register)
{
    sqlite3_stmt *stmt;
    int user_exists = 0;
    int result = 0;

    // 哈希密码
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password, strlen(password));
    SHA256_Final(hash, &sha256);

    char hashed_password[65];
    sha256_hash_string(hash, hashed_password);


    // 首先检查用户是否存在
    const char *sql_check = "SELECT Id FROM Users WHERE Username = ?";

    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user_exists = 1;
        }
        sqlite3_finalize(stmt);
    }




    // 如果用户不存在且是注册请求，则创建新用户
    if (!user_exists && is_register) {
        const char *sql_insert = "INSERT INTO Users (Username, Password) VALUES (?, ?)";
        if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, hashed_password, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                result = 1;  // 新用户注册成功
            }
            sqlite3_finalize(stmt);
        }
    } else if (user_exists && !is_register) {
        // 如果用户存在且不是注册请求，验证密码
        const char *sql_auth = "SELECT Id FROM Users WHERE Username = ? AND Password = ?";
        if (sqlite3_prepare_v2(db, sql_auth, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, hashed_password, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                result = 1;  // 用户认证成功
            }
            sqlite3_finalize(stmt);
        }
    }

    return result;
}



int add_user(const char *username, const char *password)
{
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Users (Username, Password) VALUES (?, ?)";

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password, strlen(password));
    SHA256_Final(hash, &sha256);

    char hashed_password[65];
    sha256_hash_string(hash, hashed_password);


    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, hashed_password, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            printf("Failed to add user: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return 0; // 添加失败
        }
        sqlite3_finalize(stmt);
        return 1; // 添加成功
    } else {
        printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 0; // 添加失败
    }
}


// Function to check if the user already exists in the database
int user_exists(const char *username)
{
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM users WHERE username = ?";

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Bind the username to the SQL query
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    // Execute the query
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    // Finalize the statement to prevent resource leaks
    sqlite3_finalize(stmt);

    // If count is greater than 0, then the user exists
    return count > 0;
}

void sha256_hash_string(unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

