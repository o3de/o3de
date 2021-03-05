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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ZIPENCRYPTOR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ZIPENCRYPTOR_H
#pragma once

#include "IConvertor.h"

struct ConvertContext;

class ZipEncryptor
    : public ICompiler
    , public IConvertor
{
public:

    ZipEncryptor(IResourceCompiler* pRC);
    ~ZipEncryptor();

    static bool ParseKey(uint32 outputKey[4], const char* inputString);

    // ICompiler + IConvertor methods.
    virtual void Release();

    // ICompiler methods.
    virtual void BeginProcessing([[maybe_unused]] const IConfig* config) { }
    virtual void EndProcessing() { }
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();

    // IConvertor methods.
    virtual ICompiler* CreateCompiler();
    virtual const char* GetExt(int index) const;

private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;

private:
    ConvertContext m_CC;
    int m_refCount;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ZIPENCRYPTOR_H
