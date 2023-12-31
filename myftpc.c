#include "client.h"
#include <openssl/sha.h>

void sha256_hash_string(unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65]);


int main(int argc, char **argv)
{

    if (argc != 2)
        exit_fprintf_("Usage: %s [hostname]\n", argv[0]);

    char *server_ipaddr_str = argv[1];
    unsigned int port = TCP_PORT;

    int sock;
    struct sockaddr_in saddress;
    struct ftph *buf;
    mem_alloc(buf, struct ftph, 1, EXIT_FAILURE);

    struct in_addr addr;

    memset(&saddress, 0, sizeof(saddress));
    saddress.sin_family = AF_INET;
    saddress.sin_port = htons(port);
    if(inet_pton(AF_INET, server_ipaddr_str, &saddress.sin_addr.s_addr) == 0){
        fprintf(stderr, "error: Invalid IP\n");
        return 1;
    }

    addr.s_addr = saddress.sin_addr.s_addr;
    printf("> server is  %s:%d\n", inet_ntoa(addr), ntohs(saddress.sin_port));

    printf("Prepare soket ... ");
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("failed\n"), fflush(stdout);
        exit_perror("socket");
    }

    if (connect(sock, (struct sockaddr *)&saddress, sizeof(saddress)) < 0) {
        printf("failed\n"), fflush(stdout);
        exit_perror("connect");
    }

    printf("ok\n");





    // Authentication Section
    char server_message[100];
    recv(sock, &server_message, sizeof(server_message), 0);
    printf("Server: %s\n", server_message);

    // Send username and password to server
    char username[100]; // Adjust size as needed
    char password[100]; // Adjust size as needed
    int is_register;

    printf("Enter 0 to login, 1 to register: ");
    scanf("%d", &is_register);
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    // 计算密码的SHA-256哈希
    unsigned char hash[SHA256_DIGEST_LENGTH];  //SHA256_DIGEST_LENGTH是openssl库定义的常量
    SHA256_CTX sha256;  //SHA256_CTX是openssl库定义的结构体
    SHA256_Init(&sha256); //初始化sha256
    SHA256_Update(&sha256, password, strlen(password)); //计算密码的哈希
    SHA256_Final(hash, &sha256); //将哈希值存储在hash中
    char hashed_password[65]; //存储哈希值的字符串
    sha256_hash_string(hash, hashed_password); //将哈希值转换为字符串

    //发送凭证到服务器
    send(sock,&is_register,sizeof(is_register),0);
    send(sock, username, sizeof(username), 0);
    send(sock, hashed_password, sizeof(hashed_password), 0);

    //接收服务器的认证结果
    recv(sock, &server_message, sizeof(server_message), 0);
    printf("%s\n", server_message);  //打印认证结果




    struct func_table *ft;
    char strbuf[MAX_STR_LEN];

    while (1)
    {
        // wait inputs ...
        printf("myFTP%% ");
        if (fgets(strbuf, MAX_STR_LEN, stdin) == NULL)
            continue;

        // parse inputs ...
        if (strlen(strbuf) > 0)
            strbuf[strlen(strbuf) - 1] = '\0';

        int i = 0;
        char *p = strtok(strbuf, " ");

        while (p != NULL)
        {
            strcpy(cmds[i++], p);
            p = strtok(NULL, " ");
        }
        cmds[i][0] = '\0';
        if (i == 0)
            continue;

        // search function ...
        for (ft = ftab; ft->func != NULL; ft++)
        {
            if (strcmp(ft->name, cmds[0]) == 0)
            {
                if (ft->type == NO_ARGS)
                {
                    if (i > 1)
                    {
                        fprintf(stderr, "syntax error\n");
                        break;
                    }
                }
                else if (ft->type == ARGS0or1) {
                    if (i > 2)
                    {
                        fprintf(stderr, "syntax error\n");
                        break;
                    }
                }
                else if (ft->type == ARGS1)
                {
                    if (i == 1 || i > 2)
                    {
                        fprintf(stderr, "syntax error\n");
                        break;
                    }
                }
                else if (ft->type == ARGS1or2)
                {
                    if (i == 1)
                    {
                        fprintf(stderr, "missing path\n");
                        break;
                    }
                    else if (i > 3)
                    {
                        fprintf(stderr, "syntax error\n");
                        break;
                    }
                }

                (ft->func)(i, sock, buf);
                break;
            }
        }

        // not found function ...
        if (ft->func == NULL)
            fprintf(stderr, "unknwon command: %s\n", cmds[0]);

        // reset ...
        memset(strbuf, 0, sizeof(char) * MAX_STR_LEN);
    }

    close(sock);
    free(buf);

    return 0;
}


void sha256_hash_string(unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

