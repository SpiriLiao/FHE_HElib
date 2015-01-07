#include "FHE.h"
#include "EncryptedArray.h"
#include <NTL/lzz_pXFactoring.h>
#include <fstream>
#include <sstream>
#include <sys/time.h>

#include <algorithm>
#include <iterator>

#include <sock.h>


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
        assert(keyFile.is_open());
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
//    cout << publicKey << endl;

//  输出公钥到iotest.txt
    keyFile << publicKey << endl;
//关闭文件流
    keyFile.close();

    EncryptedArray ea(context, G);  // construct an Encrypted array object ea that is
                                    // associated with the given context and polynomial G
    long nslots = ea.size();
    cout << "nslots 的大小为: " << nslots << endl;

    vector<long> v1;
    v1.push_back(0);
    v1.push_back(111);
    v1.push_back(170);
    v1.push_back(66);

// 输出向量v1的值
    cout << "向量v1的值如下:" << endl;
    copy (v1.begin(), v1.end(), ostream_iterator<long>(cout, " "));
    cout << endl;

    Ctxt ct1(publicKey);
    ea.encrypt(ct1, publicKey, v1);

    vector<long> v2;
    for (int i = 0; i < nslots; i++){
        v2.push_back(i*3);
    }
    Ctxt ct2(publicKey);
    ea.encrypt(ct2, publicKey, v2);

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

// 把密文ct1和ct2传输至服务器
    std::ostringstream oss;
    oss << ct1;
    cout << "发送给服务器的密文ct1大小为：";
    cout << oss.str().size() << endl;
    if (send(sock_fd, oss.str().c_str(), oss.str().size(), 0) < 0)
    {
        puts("\nSend failed!");
        return 1;
    }

// 这条语句很重要，发送下一次数据之前把oss对象清空，不然会出现"数据重复"
    oss.str("");
/*
    sleep(1);
    oss << ct2;
    cout << "发送给服务器的密文ct2大小为：";
    cout << oss.str().size() << endl;
    if (send(sock_fd, oss.str().c_str(), oss.str().size(), 0) < 0)
    {
        puts("\nSend failed!");
        return 1;
    }
*/
/*
// 接收服务器计算后的结果密文
    Ctxt ctSum(publicKey);
    Ctxt ctProd(publicKey);
    for (int i = 0; i < 2; ++i)
    {
        int buffer_size = 100000;
        char *buffer = new char[buffer_size];
        int bytes_read = recv(sock_fd, buffer, buffer_size, 0);
    // 输出接收到的密文
        cerr << "输出服务器计算后发送回来的的密文：" << endl;
//        puts(buffer);

        string sBuffer((const char*)buffer, bytes_read);
        cout << "收到的结果密文大小sBuffer为：" << sBuffer.length() << endl;
        delete buffer;
        std::istringstream iss;

        iss.str(sBuffer);

        if (0 == i)
        {
            iss >> ctSum;
        }
        else if (1 == i)
        {
            iss >> ctProd;
        }
    }

// 收到服务器端传回的数据进行解密

    vector<long> res;
    ea.decrypt(ctSum, secretKey, res);

    cout << "All computation are modulo " << p << "." << endl;
    for (int i = 0; i < res.size(); i++){
        cout << v1[i] << " + " << v2[i] << " = " << res[i] << endl;
    }

    ea.decrypt(ctProd, secretKey, res);
    for (int i = 0; i < res.size(); i++){
        cout << v1[i] << " * " << v2[i] << " = " << res[i] << endl;
    }

    close(sock_fd);
*/
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
    inet_aton("192.168.137.100",&cliaddr.sin_addr);

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
