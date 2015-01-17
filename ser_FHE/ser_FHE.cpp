#include "FHE.h"
#include "EncryptedArray.h"
#include <NTL/lzz_pXFactoring.h>
#include <fstream>
#include <sstream>
#include <sys/time.h>

#include <algorithm>
#include <iterator>

#include <sock.h>
#include <mysql/mysql.h>

int sock_ser_init();
void mysql_init_d(MYSQL& mysql);
int recv_data(int conn);

int main(int argc, char **argv)
{
// 套接字连接初始化
    int sock_fd =sock_ser_init();

    struct sockaddr_in  peeraddr;
    socklen_t peerlen = sizeof(peeraddr);
    int conn;
    if((conn = accept(sock_fd,(struct sockaddr*)&peeraddr,&peerlen)) < 0)
        ERR_EXIT("accept err");
    printf("客户端连接!\n");

// 接收含有FHEcontext和publicKey的iotest.txt文件
    recv_data(conn);

    fstream keyFile("iotest.txt", fstream::in);
    unsigned long m1, p1, r1;
    readContextBase(keyFile, m1, p1, r1);
    FHEcontext context(m1, p1, r1);
    keyFile >> context;
/*
    // 输出整个FHEcontext的内容
    cout << "输出整个FHEcontext的内容：" << endl;
    writeContextBase(cerr, context);
    cerr << context << endl;
*/
    FHESecKey secretKey(context);
    FHEPubKey publicKey = secretKey;
    keyFile >> publicKey;

// 构造密文数组存放用户信息
    Ctxt* ctxt[4];
    for (int i = 0; i < 4; i++)
    {
        ctxt[i] = new Ctxt(publicKey);
    }

// 初始化数据库连接
    MYSQL   mysql;
    cout << "++++++++++++++++++++++++++++" << endl;
    mysql_init_d(mysql);
    cout << "++++++++++++++++++++++++++++" << endl;

    sleep(1);
    char *ctxt_buf[4];
    int buffer_size = 100000;
    for (int i = 0; i < 4; i++)
    {
        ctxt_buf[i] = new char[buffer_size];
    }
    for (int i = 0; i < 4; i++)
    {        
        char *buffer = new char[buffer_size];
        int bytes_read = recv(conn, buffer, buffer_size, 0);
        strcpy(ctxt_buf[i], buffer);
    // 输出接收到的密文
    //    cerr << "输出接收到的密文：" << endl;
    //    puts(buffer);

        string sBuffer((const char*)buffer, bytes_read);
        cout << "收到的密文大小sBuffer为：" << sBuffer.length() << endl;
        delete buffer;
        std::istringstream iss;

        iss.str(sBuffer);
        iss >> *ctxt[i];

        sleep(1);
    }

    char *buf = new char[buffer_size];
    sprintf(buf, "insert into Persons values (1, '%s', '%s', '%s', '%s');", ctxt_buf[0], ctxt_buf[1], ctxt_buf[2], ctxt_buf[3]);
    mysql_query(&mysql, buf/*"insert into Persons values(1,'b')"*/);
    delete buf;


// 把用户信息密文返回到客户端
    for (int i = 0; i < 4; i++)
    {
        std::ostringstream oss;
        oss << *ctxt[i];
        cout << "返回客户端的和值密文大小为：";
        cout << oss.str().size() << endl;

        if (send(conn, oss.str().c_str(), oss.str().size(), 0) < 0)
        {
            puts("\nSend failed!");
            return 1;
        }
        sleep(1);

        oss.str("");
    }

    return 0;
}

int sock_ser_init()
{
    int sfd;
    if((sfd = socket(PF_INET,SOCK_STREAM,0)) < 0)
        ERR_EXIT("socket err");

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(4567);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1")
    //inet_aton("127.0.0.1",&servaddr.sin_addr);

    int on=1;
    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) < 0)
        ERR_EXIT("setsockopt err");

    if(bind(sfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0)
        ERR_EXIT("bind err");

    if(listen(sfd,SOMAXCONN) < 0)
        ERR_EXIT("listen err");
    return sfd;
}

//接收文件名,
int recv_filename(int conn,char** name)
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

// 接收数据
int recv_data(int conn)
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
    fclose(fp);
    printf("%s 文件上传成功\n",file_name);
}

void mysql_init_d(MYSQL& mysql)
{
    MYSQL_RES   *result;
    MYSQL_ROW   row;

    //initialize   MYSQL   structure
    mysql_init(&mysql);
    //connect   to   database
    //   mysql_real_connect(&mysql,"localhost","root","nriet","test",0,NULL,0);
    if (!mysql_real_connect(&mysql,"127.0.0.1","root","123456","fhe",0,NULL,0))
    {
        printf("%s", mysql_error(&mysql));
    }
     else
    {
        printf("Database Conected succeed!\n");
    }
}



































































































