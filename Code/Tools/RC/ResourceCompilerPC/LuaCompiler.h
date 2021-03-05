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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_LUACOMPILER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_LUACOMPILER_H
#pragma once


#include "IConvertor.h"

struct ConvertContext;

class LuaCompiler
    : public IConvertor
    , public ICompiler
{
public:
    LuaCompiler();
    ~LuaCompiler();

    // IConvertor methods.
    virtual ICompiler* CreateCompiler();
    virtual const char* GetExt(int index) const { return (index == 0) ? "lua" : 0; }

    // ICompiler methods.
    virtual void BeginProcessing([[maybe_unused]] const IConfig* config) { }
    virtual void EndProcessing() { }
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();

    // ICompiler + IConvertor methods.
    void Release();

public:
    bool IsDumping() const { return m_bIsDumping; }
    bool IsStripping() const { return m_bIsStripping; }
    bool IsBigEndian() const { return m_bIsBigEndian; }
    const char* GetInFilename() const { return m_sInFilename.c_str(); }
    const char* GetOutFilename() const { return m_sOutFilename.c_str(); }

private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;

private:
    int m_refCount;

private:
    bool m_bIsDumping;          /* dump bytecodes? */
    bool m_bIsStripping;            /* strip debug information? */
    bool m_bIsBigEndian;
    string m_sInFilename;
    string m_sOutFilename;
    ConvertContext m_CC;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_LUACOMPILER_H
