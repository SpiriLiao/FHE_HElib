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
    m = 100;


    int sock_fd = sock_cli_init();

    FHEcontext context(m, p, r);    // initialize context

// 输出整个FHEcontext的内容
    writeContextBase(cerr, context);
    cerr << context << endl;

    buildModChain(context, L, c);   // modify the context, adding primes to the modulus chain
    FHESecKey secretKey(context);   // contruct a secret key structure

// 输出私钥
    cout << "输出私钥：" << endl;
    cerr << secretKey << endl;

    const FHEPubKey& publicKey = secretKey; // an "upcast": FHESecKey is a subclass of FHEPubKey

// 输出公钥
    cout << "输出公钥：" << endl;
    cerr << publicKey << endl;

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

    EncryptedArray ea(context, G);  // construct an Encrypted array object ea that is
                                    // associated with the given context and polynomial G

    long nslots = ea.size();
    cout << "nslots 的大小为: " << nslots << endl;

    vector<long> v1;
    for (int i = 0; i < nslots; i++){
        v1.push_back(i*4);
    }
// 输出向量v1的值
    cout << "向量v1的值如下:" << endl;
//    copy (v1.begin(), v1.end(), ostream_iterator<long>(cout, " "));
    cout << endl;

    Ctxt ct1(publicKey);
    ea.encrypt(ct1, publicKey, v1);
// 输出密文c1
    cerr << "密文c1如下:" << endl;
//    cerr << ct1 << endl;

    vector<long> v2;
    for (int i = 0; i < nslots; i++){
        v2.push_back(5);
    }
    Ctxt ct2(publicKey);
    ea.encrypt(ct2, publicKey, v2);
// 输出密文c2
    cerr << "密文c2如下:" << endl;
//    cout << ct2 << endl;

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

    sleep(1);
// 这条语句很重要，发送下一次数据之前把oss对象清空，不然会出现"数据重复"
    oss.str("");

    sleep(1);
    oss << ct2;
    cout << "发送给服务器的密文ct2大小为：";
    cout << oss.str().size() << endl;
    if (send(sock_fd, oss.str().c_str(), oss.str().size(), 0) < 0)
    {
        puts("\nSend failed!");
        return 1;
    }

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
/*
    cout << "向量res的值如下:" << endl;
    copy (res.begin(), res.end(), ostream_iterator<long>(cout, " "));
    cout << endl;
*/
    cout << "All computation are modulo " << p << "." << endl;
    for (int i = 0; i < res.size(); i++){
        cout << v1[i] << " + " << v2[i] << " = " << res[i] << endl;
    }

    ea.decrypt(ctProd, secretKey, res);
    for (int i = 0; i < res.size(); i++){
        cout << v1[i] << " * " << v2[i] << " = " << res[i] << endl;
    }

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
    inet_aton("192.168.137.100",&cliaddr.sin_addr);

    if( connect(sfd,(struct sockaddr*)&cliaddr,sizeof(cliaddr)) < 0)
        ERR_EXIT("connect err");

    return sfd;
}
