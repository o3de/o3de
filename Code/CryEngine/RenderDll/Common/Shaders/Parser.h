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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

struct STokenDesc
{
    int  id;
    const char* token;
};
int shGetObject(char** buf, STokenDesc* tokens, char** name, char** data);


extern const char* kWhiteSpace;
extern FXMacro sStaticMacros;

bool SkipChar(unsigned int ch);

void fxParserInit(void);

void SkipCharacters(char** buf, const char* toSkip);
void RemoveCR(char* buf);
void SkipComments(char** buf, bool bSkipWhiteSpace);

bool fxIsFirstPass(char* buf);
void fxRegisterEnv(const char* szStr);

int shFill(char** buf, char* dst, int nSize = -1);
int fxFill(char** buf, char* dst, int nSize = -1);
char* fxFillPr (char** buf, char* dst);
char* fxFillPrC (char** buf, char* dst);
char* fxFillNumber (char** buf, char* dst);
int fxFillCR(char** buf, char* dst);

bool shGetBool(char* buf);
float shGetFloat(const char* buf);
void shGetFloat(const char* buf, float* v1, float* v2);
int shGetInt(const char* buf);
int shGetHex(const char* buf);
uint64 shGetHex64(const char* buf);
void shGetVector(char* buf, Vec3& v);
void shGetVector(char* buf, float v[3]);
void shGetVector4(char* buf, vec4_t& v);
void shGetColor(const char* buf, ColorF& v);
void shGetColor(char* buf, float v[4]);
int shGetVar (char** buf, char** vr, char** val);
