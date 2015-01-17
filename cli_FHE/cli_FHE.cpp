#include "FHE.h"
#include "EncryptedArray.h"
#include <NTL/lzz_pXFactoring.h>
#include <fstream>
#include <sstream>
#include <sys/time.h>

#include <algorithm>
#include <iterator>

#include <sock.h>
#include <QString>
#include <QDebug>


int sock_cli_init();
int send_data(int sfd, char* filename);

int main(int argc, char **argv)
{
    /* On our trusted system we generate a new key
     * (or read one in) and encrypt the secret data set.
    */

    long m = 0, p = 715827883, r = 1;   // Native plaintext space(本机明文空间)
                                // Computations will be 'modulo p'
    long L = 16;                // Levels
    long c = 3;                 // Columns in key switching matrix
    long w = 64;                // Hamming weight of secret key
    long d = 0;
    long security = 128;
    ZZX G;
//    m = FindM(security, L, c, p, d, 0, 0);
    m = 212;

    int sock_fd = sock_cli_init();

    FHEcontext context(m, p, r);    // initialize context
    buildModChain(context, L, c);   // modify the context, adding primes to the modulus chain

// 新建iotest.txt保存FHEcontext和publicKey
    fstream keyFile("iotest.txt", fstream::out|fstream::trunc);
//        assert(keyFile.is_open());
// 输出FHEcontext到iotest.txt
    writeContextBase(keyFile, context);
    keyFile << context << endl;

/*
// 输出整个FHEcontext的内容
    cout << "输出整个FHEcontext的内容：" << endl;
    writeContextBase(cerr, context);
    cerr << context << endl;
*/
    FHESecKey secretKey(context);   // contruct a secret key structure
    const FHEPubKey& publicKey = secretKey; // an "upcast": FHESecKey is a subclass of FHEPubKey

    if (0 == d)
    G = context.alMod.getFactorsOverZZ()[0];

    secretKey.GenSecKey(w);         // actually generate a secret key with Hamming weight w

    addSome1DMatrices(secretKey);
    cout << "Generated key" << endl;

// 输出私钥
    cout << "输出私钥：" << endl;
//    cout << secretKey << endl;

// 输出公钥
    cout << "输出公钥：" << endl;
    cout << publicKey << endl;

//  输出公钥到iotest.txt
    keyFile << publicKey << endl;
//关闭文件流
    keyFile.close();

    EncryptedArray ea(context, G);  // construct an Encrypted array object ea that is
                                    // associated with the given context and polynomial G
    long nslots = ea.size();
    cout << "nslots 的大小为: " << nslots << endl;

// 姓名
    cout << endl << "请输入名字:";
    string name;
    cin >> name;

    vector<long> vec_name;
    vec_name.push_back(name.operator [](0));
    vec_name.push_back(name.operator [](1));
    vec_name.push_back(name.operator [](2));
    vec_name.push_back('a');

// 性别
    cout << endl << "请输入性别:";
    string gender;
    cin >> gender;

    vector<long> vec_gender;
    vec_gender.push_back(gender.operator [](0));
    vec_gender.push_back(gender.operator [](1));
    vec_gender.push_back(gender.operator [](2));
    vec_gender.push_back('a');

// 身高
    cout << endl << "请输入身高:";
    string height;
    cin >> height;

    vector<long> vec_height;
    vec_height.push_back(height.operator [](0));
    vec_height.push_back(height.operator [](1));
    vec_height.push_back(height.operator [](2));
    vec_height.push_back('a');

// 体重
    cout << endl << "请输入体重:";
    string weight;
    cin >> weight;

    vector<long> vec_weight;
    vec_weight.push_back(weight.operator [](0));
    vec_weight.push_back(weight.operator [](1));
    vec_weight.push_back(weight.operator [](2));
    vec_weight.push_back('a');

// 构造密文数组存放用户信息
    Ctxt* ctxt[4];
    for (int i = 0; i < 4; i++)
    {
        ctxt[i] = new Ctxt(publicKey);
    }
    ea.encrypt(*ctxt[0], publicKey, vec_name);
    ea.encrypt(*ctxt[1], publicKey, vec_gender);
    ea.encrypt(*ctxt[2], publicKey, vec_height);
    ea.encrypt(*ctxt[3], publicKey, vec_weight);

// 把含有FHEcontext和publicKey的iotest.txt发送给服务器
    FILE *fp = fopen("iotest.txt", "r");// 只读方式打开
    cout << "+++++++++++++++++++++++" << endl;
    if(NULL == fp)
    {
        printf("文件不存在");
        return 1;
    }
    send_data(sock_fd, "iotest.txt");
    cout << "+++++++++++++++++++++++" << endl;

// 把用户信息密文传输至服务器
    sleep(1);
    for (int i = 0; i < 4; ++i)
    {
        std::ostringstream oss;
        oss << *ctxt[i];
        cout << "发送给服务器的密文大小为：";
        cout << oss.str().size() << endl;
        if (send(sock_fd, oss.str().c_str(), oss.str().size(), 0) < 0)
        {
            puts("\nSend failed!");
            return 1;
        }

    // 这条语句很重要，发送下一次数据之前把oss对象清空，不然会出现"数据重复"
        oss.str("");
        sleep(1);
    }

// 接收客户端的处理信息
    Ctxt* ct_res[4];
    for (int i = 0; i < 4; i++)
    {
        ct_res[i] = new Ctxt(publicKey);
    }

    for (int i = 0; i < 4; i++)
    {
        int buffer_size = 100000;
        char *buffer = new char[buffer_size];
        int bytes_read = recv(sock_fd, buffer, buffer_size, 0);
    // 输出接收到的密文
    //    cerr << "输出服务器计算后发送回来的的密文：" << endl;
    //        puts(buffer);

        string sBuffer((const char*)buffer, bytes_read);
        cout << "收到的结果密文大小sBuffer为：" << sBuffer.length() << endl;
        delete buffer;
        std::istringstream iss;

        iss.str(sBuffer);

        iss >> *ct_res[i];
    }

    vector<long> res[4];
    for (int i = 0; i < 4; i++)
    {
        ea.decrypt(*ct_res[i], secretKey, res[i]);
    }
// 输出向量res的值
    cout << "向量res的值如下(用户名):" << endl;
    copy (res[2].begin(), res[2].end(), ostream_iterator<long>(cout, " "));
    cout << endl;

    QString str_res;
    str_res.append((char )res[2].operator [](0));
    str_res.append((char )res[2].operator [](1));
    str_res.append((char )res[2].operator [](2));

    qDebug() << str_res << endl;

    close(sock_fd);

    return 0;
}

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
    inet_aton("192.168.137.33",&cliaddr.sin_addr);

    if( connect(sfd,(struct sockaddr*)&cliaddr,sizeof(cliaddr)) < 0)
        ERR_EXIT("connect err");

    return sfd;
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

// 发送
int send_data(int sfd, char* filename)
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
