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

#include "ResourceCompiler_precompiled.h"
#include <time.h>
#include "ExtensionManager.h"
#include "IConvertor.h"
#include "IResCompiler.h"       // IResourceCompiler
#include "IRCLog.h"             // IRCLog
#include "StringHelpers.h"

//////////////////////////////////////////////////////////////////////////
ExtensionManager::ExtensionManager()
{
}

//////////////////////////////////////////////////////////////////////////
ExtensionManager::~ExtensionManager()
{
    UnregisterAll();
}

//////////////////////////////////////////////////////////////////////////
IConvertor* ExtensionManager::FindConvertor(const char* filename)  const
{
    string const strFilename = StringHelpers::MakeLowerCase(string(filename));

    for (size_t i = 0; i < m_extVector.size(); ++i)
    {
        if (StringHelpers::EndsWith(strFilename, m_extVector[i].first))
        {
            return m_extVector[i].second;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void ExtensionManager::RegisterConvertor(const char* name, IConvertor* conv, [[maybe_unused]] IResourceCompiler* rc)
{
    assert(conv);
    assert(rc);

    m_convertors.push_back(conv);

    string strExt;
    for (int i = 0;; ++i)
    {
        const char* const ext = conv->GetExt(i);
        if (!ext || !ext[0])
        {
            break;
        }
        if (i)
        {
            strExt += ", ";
        }
        strExt += "\"";
        strExt += ext;
        strExt += "\"";

        const string extToAdd = StringHelpers::MakeLowerCase(string(".") + ext);
        m_extVector.push_back(ExtVector::value_type(extToAdd, conv));
    }

    if (strExt.empty())
    {
        RCLogError("    %s failed to provide list of extensions", name);
    }
    else
    {
        RCLog("    Registered %s (%s)", name, strExt.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void ExtensionManager::UnregisterAll()
{
    for (size_t i = 0; i < m_convertors.size(); ++i)
    {
        IConvertor* conv = m_convertors[i];
        conv->Release();
    }
    m_convertors.clear();

    m_extVector.clear();
}
