/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef CRYINCLUDE_CRYPTOGRAPHY_RIJNDAEL_H
#define CRYINCLUDE_CRYPTOGRAPHY_RIJNDAEL_H
#pragma once

#include <ICrypto.h>

//
// File : rijndael.h
// Creation date : Sun Nov 5 2000 03:21:05 CEST
// Author : Szymon Stefanek (stefanek@tin.it)
//
// Another implementation of the Rijndael cipher.
// This is intended to be an easily usable library file.
// This code is public domain.
// Based on the Vincent Rijmen and K.U.Leuven implementation 2.4.
//

//
// Original Copyright notice:
//
//    rijndael-alg-fst.c   v2.4   April '2000
//    rijndael-alg-fst.h
//    rijndael-api-fst.c
//    rijndael-api-fst.h
//
//    Optimised ANSI C code
//
//    authors: v1.0: Antoon Bosselaers
//             v2.0: Vincent Rijmen, K.U.Leuven
//             v2.3: Paulo Barreto
//             v2.4: Vincent Rijmen, K.U.Leuven
//
//    This code is placed in the public domain.
//

//
// This implementation works on 128 , 192 , 256 bit keys
// and on 128 bit blocks
//

//
// Example of usage:
//
//  // Input data
//  unsigned char key[32];                       // The key
//  initializeYour256BitKey();                   // Obviously initialized with sth
//  const unsigned char * plainText = getYourPlainText(); // Your plain text
//  int plainTextLen = strlen(plainText);        // Plain text length
//
//  // Encrypting
//  Rijndael rin;
//  unsigned char output[plainTextLen + 16];
//
//  rin.init(Rijndael::CBC,Rijndael::Encrypt,key,Rijndael::Key32Bytes);
//  // It is a good idea to check the error code
//  int len = rin.padEncrypt(plainText,len,output);
//  if(len >= 0)useYourEncryptedText();
//  else encryptError(len);
//
//  // Decrypting: we can reuse the same object
//  unsigned char output2[len];
//  rin.init(Rijndael::CBC,Rijndael::Decrypt,key,Rijndael::Key32Bytes));
//  len = rin.padDecrypt(output,len,output2);
//  if(len >= 0)useYourDecryptedText();
//  else decryptError(len);
//

class Rijndael
    : public IRijndael
{
public:
    //////////////////////////////////////////////////////////////////////////////////////////
    // API
    //////////////////////////////////////////////////////////////////////////////////////////

    // init(): Initializes the crypt session
    // Returns RIJNDAEL_SUCCESS or an error code
    // mode      : Rijndael::ECB, Rijndael::CBC or Rijndael::CFB1
    //             You have to use the same mode for encrypting and decrypting
    // dir       : Rijndael::Encrypt or Rijndael::Decrypt
    //             A cipher instance works only in one direction
    //             (Well , it could be easily modified to work in both
    //             directions with a single init() call, but it looks
    //             useless to me...anyway , it is a matter of generating
    //             two expanded keys)
    // key       : array of unsigned octets , it can be 16 , 24 or 32 bytes long
    //             this CAN be binary data (it is not expected to be null terminated)
    // keyLen    : Rijndael::Key16Bytes , Rijndael::Key24Bytes or Rijndael::Key32Bytes
    // initVector: initialization vector, you will usually use 0 here
    int init(RijndaelState& state, RijndaelMode mode, RijndaelDirection dir, const uint8* key, RijndaelKeyLength keyLen, uint8* initVector = 0);
    // Encrypts the input array (can be binary data)
    // The input array length must be a multiple of 16 bytes, the remaining part
    // is DISCARDED.
    // so it actually encrypts inputLen / 128 blocks of input and puts it in outBuffer
    // Input len is in BITS!
    // outBuffer must be at least inputLen / 8 bytes long.
    // Returns the encrypted buffer length in BITS or an error code < 0 in case of error
    int blockEncrypt(RijndaelState& state, const uint8* input, int inputLen, uint8* outBuffer);
    // Encrypts the input array (can be binary data)
    // The input array can be any length , it is automatically padded on a 16 byte boundary.
    // Input len is in BYTES!
    // outBuffer must be at least (inputLen + 16) bytes long
    // Returns the encrypted buffer length in BYTES or an error code < 0 in case of error
    int padEncrypt(RijndaelState& state, const uint8* input, int inputOctets, uint8* outBuffer);
    // Decrypts the input vector
    // Input len is in BITS!
    // outBuffer must be at least inputLen / 8 bytes long
    // Returns the decrypted buffer length in BITS and an error code < 0 in case of error
    int blockDecrypt(RijndaelState& state, const uint8* input, int inputLen, uint8* outBuffer);
    // Decrypts the input vector
    // Input len is in BYTES!
    // outBuffer must be at least inputLen bytes long
    // Returns the decrypted buffer length in BYTES and an error code < 0 in case of error
    int padDecrypt(RijndaelState& state, const uint8* input, int inputOctets, uint8* outBuffer);
protected:
    void keySched(RijndaelState & state, uint8 key[_MAX_KEY_COLUMNS][4]);
    void keyEncToDec(RijndaelState& state);
    void encrypt(RijndaelState & state, const uint8 a[16], uint8 b[16]);
    void decrypt(RijndaelState & state, const uint8 a[16], uint8 b[16]);
};

#endif // CRYINCLUDE_CRYPTOGRAPHY_RIJNDAEL_H
