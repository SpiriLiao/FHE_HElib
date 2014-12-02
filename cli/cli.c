///cli.c
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define ERR_EXIT(m)\
    do{\
    perror(m);\
    exit(EXIT_FAILURE);\
    }while(0)



int sock_cli_init()
{
    int sfd;
    if( (sfd = socket(PF_INET,SOCK_STREAM,0)) < 0)
        ERR_EXIT("socket err");

    struct sockaddr_in cliaddr;
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(4567);
//    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
//    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1")
    inet_aton("192.168.137.100",&cliaddr.sin_addr);

    if( connect(sfd,(struct sockaddr*)&cliaddr,sizeof(cliaddr)) < 0)
        ERR_EXIT("connect err");

    return sfd;
}


/// 发送
int send_date(int sfd, char* filename)
{
    int len=0;
    char buf[128] = {};
    char buffer[1024] = {0};

    strcpy(buf,filename);
    len = strlen(buf);

    // 发送文件名的长度,发送文件名
    send(sfd,&len,sizeof(len),0);
    send(sfd,buf,len,0);

    // 只读方式打开
    FILE *fp = fopen(buf, "r");
    if(NULL == fp)
    {
        ERR_EXIT("open err 可能文件不存在");
    }

    else
    {

        bzero(buffer,sizeof(buffer));
        int length = 0;
        // 循环发送数据,直到文件读完为止
        while((length = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)
        {

            // 发送数据包的大小
            //printf("length = %d\n",length);

            if( send(sfd,&length,sizeof(length),0) < 0)
            {
                printf("%s 发送失败!\n", filename);
                break;
            }
            // 发送数据包的内容
            if(send(sfd, buffer, length, 0) < 0)
            {
                printf("%s 发送失败!\n", filename);
                break;
            }
            bzero(buffer,sizeof(buffer));
        }
        // 读取文件完成,发送0数据包
        length =0;
        send(sfd,&length,sizeof(length),0);
    }
    // 关闭文件
    fclose(fp);
    printf("%s 发送成功!\n", filename);
    return 0;
}


// 接收

int recv_date(int conn)
{

    int file_len = 0;
    char buffer[1024]={0};
    char file_name[128]={0};

    if(recv(conn,&file_len,sizeof(file_len),MSG_WAITALL) < 0)
        return -1;
    if(recv(conn,file_name,file_len,MSG_WAITALL) < 0)
        return -1;
    file_name[file_len] = 0;// 接收到文件名后加/0
    printf("%s文件创建成功\n",file_name);

    // 以可写方式创建文件
    FILE *fp = fopen(file_name, "w");
    if(fp == NULL)
        ERR_EXIT("fopen err");

    bzero(buffer, sizeof(buffer));
    int length = 0;
    int r;

    while(1)
    {
        // 接收数据包的大小
        recv(conn,&length,sizeof(length),MSG_WAITALL);
        //printf("length = %d\n",length);

        if(length == 0)
            break;// 当长度为0时,接收完毕,退出

        // 接收数据包的内容
        recv(conn,buffer,length,MSG_WAITALL);


        if(fwrite(buffer, sizeof(char), length, fp) < length)
        {
            printf("%s:\t 写入失败\n", file_name);
            return -1;
        }
        bzero(buffer, sizeof(buffer));


    }
    printf("%s 文件下载成功\n",file_name);


}


// 接收文件名
int   recv_filename(int conn,char** name)
{	
    int file_len =0;
    int r=0;

    if((r=recv(conn,&file_len,sizeof(file_len),MSG_WAITALL)) <= 0)
        exit(1);
    if((r = recv(conn,(*name),file_len,MSG_WAITALL)) <= 0)
        exit(1);
    (*name)[file_len] = 0;  // 接收到文件名后加/0
    //printf("// %s //\n",*name);

    return  0;
}

// 发送文件名
int send_filename(int sfd,char* filename)
{
    int len=0;
    char buf[128] = {0};

    strcpy(buf,filename);
    len = strlen(buf);
    //printf("%d\n",len);
    send(sfd,&len,sizeof(len),0);
    send(sfd,buf,len,0);
}

int main()
{

    int sock_fd = sock_cli_init();

    int witch=0;

    while(1)
    {
        int witch=0;
        printf("**************************************\n");
        printf("  1:上传文件 \n  2:下载文件 \n  3:关闭客户端\n");
        printf("**************************************\n");
        printf("请输入你需要的操作选项:");

        scanf("%d",&witch);

        if(witch==1 || witch==2 || witch==3 )
            send(sock_fd,&witch,sizeof(witch),0);

        switch(witch)
        {
        case 1:
        {
            char filename[128]={0};
            printf("请输入你要上传的文件名:");
            scanf("%s",filename);
            FILE *fp = fopen(filename, "r");// 只读方式打开
            if(NULL == fp)
            {
                printf("文件不存在");
                break;
            }
            send_date(sock_fd, filename);
            break;
        }
        case 2:
        {

            char filename[128]={0};
            printf("请输入你要下载的文件名:");
            scanf("%s",filename);
            send_filename(sock_fd,filename);


            recv_date(sock_fd);
            break;
        }
        case 3:
        {
            break;
        }
        default:
            break;
        }
        if(witch == 3)
            break;

    }

    close( sock_fd);
    return 0;
}



