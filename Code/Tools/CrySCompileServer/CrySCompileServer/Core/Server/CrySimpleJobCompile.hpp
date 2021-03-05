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

#ifndef __CRYSIMPLEJOBCOMPILE__
#define __CRYSIMPLEJOBCOMPILE__

#include "CrySimpleJobCache.hpp"
#include <Core/Common.h>
#include <Core/Error.hpp>


class CCrySimpleJobCompile
    :   public  CCrySimpleJobCache
{
public:
    CCrySimpleJobCompile(uint32_t requestIP, EProtocolVersion Version, std::vector<uint8_t>* pRVec);
    virtual                                     ~CCrySimpleJobCompile();

    virtual bool                            Execute(const TiXmlElement* pElement);

    static long            GlobalCompileTasks(){return m_GlobalCompileTasks; }
    static long            GlobalCompileTasksMax(){return m_GlobalCompileTasksMax; }

private:
    static AZStd::atomic_long             m_GlobalCompileTasks;
    static AZStd::atomic_long             m_GlobalCompileTasksMax;
    static volatile int32_t             m_RemoteServerID;
    static volatile int64_t m_GlobalCompileTime;

    EProtocolVersion                    m_Version;
    std::vector<uint8_t>*           m_pRVec;

    virtual size_t                      SizeOf(std::vector<uint8_t>& rVec) = 0;

    bool                                            Compile(const TiXmlElement* pElement, std::vector<uint8_t>& rVec);
};

class CCompilerError
    : public ICryError
{
public:
    CCompilerError(const std::string& entry, const std::string& errortext, const std::string& ccs, const std::string& IP,
        const std::string& requestLine, const std::string& program, const std::string& project,
        const std::string& platform, const std::string& compiler, const std::string& language, const std::string& tags, const std::string& profile);

    virtual ~CCompilerError() {}

    virtual void AddDuplicate(ICryError* err);

    virtual void SetUniqueID(int uniqueID) { m_uniqueID = uniqueID; }

    virtual bool Compare(const ICryError* err) const;
    virtual bool CanMerge(const ICryError* err) const;

    virtual bool HasFile() const { return true; }

    virtual void AddCCs(std::set<std::string>& ccs) const;

    virtual std::string GetErrorName() const;
    virtual std::string GetErrorDetails(EOutputFormatType outputType) const;
    virtual std::string GetFilename() const { return m_entry + ".txt"; }
    virtual std::string GetFileContents() const { return m_program; }

    std::vector<std::string> m_requests;
private:
    void Init();
    std::string GetErrorLines() const;
    std::string GetContext(int linenum, int context = 2, std::string prefix = ">") const;

    std::vector< std::pair<int, std::string> > m_errors;

    tdEntryVec m_CCs;

    std::string m_entry, m_errortext, m_hasherrors, m_IP,
                m_program, m_project, m_shader,
                m_platform, m_compiler, m_language, m_tags, m_profile;
    int m_uniqueID;

    friend CCompilerError;
};

#endif
