#include "FHE.h"
#include "EncryptedArray.h"
#include <NTL/lzz_pXFactoring.h>
#include <fstream>
#include <sstream>
#include <sys/time.h>

#include <algorithm>
#include <iterator>

#include <sock.h>

int sock_ser_init();

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


    FHEcontext context(m, p, r);    // initialize context

// 输出整个FHEcontext的内容
    cout << "输出整个FHEcontext的内容：" << endl;
    writeContextBase(cout, context);
    cout << context << endl;

    buildModChain(context, L, c);   // modify the context, adding primes to the modulus chain
    FHESecKey secretKey(context);   // contruct a secret key structure

// 输出密钥
    cout << "输出密钥：" << endl;
    cout << secretKey << endl;

    const FHEPubKey& publicKey = secretKey; // an "upcast": FHESecKey is a subclass of FHEPubKey

// 输出公钥
    cout << "输出公钥：" << endl;
    cout << publicKey << endl;

    if (0 == d)
    G = context.alMod.getFactorsOverZZ()[0];

    secretKey.GenSecKey(w);         // actually generate a secret key with Hamming weight w

    addSome1DMatrices(secretKey);
    cout << "Generated key" << endl;

    EncryptedArray ea(context, G);  // construct an Encrypted array object ea that is
                                    // associated with the given context and polynomial G

// 套接字接收数据
    int sock_fd =sock_ser_init();

    struct sockaddr_in  peeraddr;
    socklen_t peerlen = sizeof(peeraddr);
    int conn;
    if((conn = accept(sock_fd,(struct sockaddr*)&peeraddr,&peerlen)) < 0)
        ERR_EXIT("accept err");
    printf("客户端连接!\n");



// 构造密文ct1和ct2
    Ctxt ct1(publicKey);
    Ctxt ct2(publicKey);

    for (int i = 0; i < 2; ++i)
    {
    int buffer_size = 100000;
    char *buffer = new char[buffer_size];
    int bytes_read = recv(conn, buffer, buffer_size, 0);
// 输出接收到的密文
//    cerr << "输出接收到的密文：" << endl;
    puts(buffer);

    string sBuffer((const char*)buffer, bytes_read);
    cout << "收到的密文大小sBuffer为：" << sBuffer.length() << endl;
    delete buffer;
    std::istringstream iss;

    iss.str(sBuffer);

    if (0 == i)
    {
        iss >> ct1;
    }
    else if (1 == i)
    {
        iss >> ct2;
    }
    }



    Ctxt ctSum = ct1;
    Ctxt ctProd = ct1;      // Product(乘积，作品)

    ctSum += ct2;
    ctProd *= ct2;

/*
// 把计算后的结果密文传回客户端
    std::ostringstream oss;
    oss << ctSum;
    cout << "返回客户端的和值密文大小为：" << endl;
    cout << oss.str().size() << endl;

    if (send(conn, oss.str().c_str(), oss.str().size(), 0) < 0)
    {
        puts("\nSend failed!");
        return 1;
    }
    sleep(1);
    oss << ctProd;
    cout << "返回客户端的乘积密文大小为：" << endl;
    cout << oss.str().size() << endl;
    if (send(conn, oss.str().c_str(), oss.str().size(), 0) < 0)
    {
        puts("\nSend failed!");
        return 1;
    }
*/

/*
    if (0 == d)
    G = context.alMod.getFactorsOverZZ()[0];

    secretKey.GenSecKey(w);         // actually generate a secret key with Hamming weight w

    addSome1DMatrices(secretKey);
    cout << "Generated key" << endl;

    EncryptedArray ea(context, G);  // construct an Encrypted array object ea that is
                                    // associated with the given context and polynomial G

    long nslots = ea.size();
    cout << "nslots 的大小为: " << nslots << endl;

    vector<long> v1;
    for (int i = 0; i < nslots; i++){
        v1.push_back(i*3);
    }
    Ctxt ct1(publicKey);
    ea.encrypt(ct1, publicKey, v1);
// 输出密文c1
    cerr << "密文c1如下:" << endl;
    cerr << ct1 << endl;

    vector<long> v2;
    for (int i = 0; i < nslots; i++){
        v2.push_back(i);
    }
    Ctxt ct2(publicKey);
    ea.encrypt(ct2, publicKey, v2);
// 输出密文c2
    cerr << "密文c2如下:" << endl;
    cerr << ct2 << endl;

    cout << "向量v1的值如下:" << endl;
    copy (v1.begin(), v1.end(), ostream_iterator<long>(cout, " "));
    cout << endl;
*/


/*
    Ctxt ctSum = ct1;
    Ctxt ctProd = ct1;      // Product(乘积，作品)

    ctSum += ct2;
    ctProd *= ct2;
*/


    vector<long> res;
    ea.decrypt(ctSum, secretKey, res);
/*
    cout << "向量res的值如下:" << endl;
    copy (res.begin(), res.end(), ostream_iterator<long>(cout, " "));
    cout << endl;
*/
    cout << "All computation are modulo " << p << "." << endl;
    for (int i = 0; i < res.size(); i++){
        cout /*<< v1[i] << " + " << v2[i] << " = " */<< res[i] << endl;
    }

    ea.decrypt(ctProd, secretKey, res);
    for (int i = 0; i < res.size(); i++){
        cout /*<< v1[i] << " * " << v2[i] << " = " */<< res[i] << endl;
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




































































































