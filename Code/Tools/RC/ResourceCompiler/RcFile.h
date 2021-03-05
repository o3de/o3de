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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_RCFILE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_RCFILE_H
#pragma once

class RcFile
{
public:
    string m_sourceLeftPath;
    string m_sourceInnerPathAndName;
    string m_targetLeftPath;

public:
    RcFile()
    {
    }

    RcFile(const string& sourceLeftPath, const string& sourceInnerPathAndName, const string& targetLeftPath)
        : m_sourceLeftPath(sourceLeftPath)
        , m_sourceInnerPathAndName(sourceInnerPathAndName)
        , m_targetLeftPath(targetLeftPath.empty() ? sourceLeftPath : targetLeftPath)
    {
    }
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_RCFILE_H
