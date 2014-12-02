#include "FHE.h"
#include "EncryptedArray.h"
#include <NTL/lzz_pXFactoring.h>
#include <fstream>
#include <sstream>
#include <sys/time.h>

#include <algorithm>
#include <iterator>

int main(int argc, char **argv)
{
    /* On our trusted system we generate a new key
     * (or read one in) and encrypt the secret data set.
    */    

    long m = 0, p = 257, r = 1;   // Native plaintext space(本机明文空间)
                                // Computations will be 'modulo p'
    long L = 16;                // Levels
    long c = 3;                 // Columns in key switching matrix
    long w = 64;                // Hamming weight of secret key
    long d = 0;
    long security = 128;
    ZZX G;
//    m = FindM(security, L, c, p, d, 0, 0);
    m = 256;

    FHEcontext context(m, p, r);    // initialize context

// 输出整个FHEcontext的内容
    writeContextBase(cerr, context);
    cerr << context << endl;

    buildModChain(context, L, c);   // modify the context, adding primes to the modulus chain
    FHESecKey secretKey(context);   // contruct a secret key structure    

// 输出密钥
    cerr << secretKey << endl;

    const FHEPubKey& publicKey = secretKey; // an "upcast": FHESecKey is a subclass of FHEPubKey

// 输出公钥
    cerr << publicKey << endl;

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

    Ctxt ctSum = ct1;
    Ctxt ctProd = ct1;      // Product(乘积，作品)

    ctSum += ct2;
    ctProd *= ct2;

    vector<long> res;
    ea.decrypt(ctSum, secretKey, res);

    cout << "向量res的值如下:" << endl;
    copy (res.begin(), res.end(), ostream_iterator<long>(cout, " "));
    cout << endl;

    cout << "All computation are modulo " << p << "." << endl;
    for (int i = 0; i < res.size(); i++){
        cout << v1[i] << " + " << v2[i] << " = " << res[i] << endl;
    }

    ea.decrypt(ctProd, secretKey, res);
    for (int i = 0; i < res.size(); i++){
        cout << v1[i] << " * " << v2[i] << " = " << res[i] << endl;
    }

    return 0;
}




































































































