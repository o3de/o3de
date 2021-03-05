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
#ifndef CRYINCLUDE_CRYACTION_JOININ_JOININCVARS_H
#define CRYINCLUDE_CRYACTION_JOININ_JOININCVARS_H
#pragma once

// This class handles registering and unregistering the JoinIn specific CVars.
// It also provides simple access to the values via typed accessors.
//
// The lifetime of the class is managed via shared_ptr; the vars are destroyed
// when there are no longer any references to JoinInCVars.
//

class JoinInCVars
{
public:
    static AZStd::shared_ptr<JoinInCVars> GetInstance();
    virtual ~JoinInCVars() = 0;

    const char* GetURIScheme() { return m_uriScheme; }

    virtual void RegisterCVars() = 0;
    virtual void UnregisterCVars() = 0;

protected:

    const char* m_uriScheme;

    // Not for public use
    JoinInCVars();
};

inline JoinInCVars::~JoinInCVars() {}

#endif // CRYINCLUDE_CRYACTION_JOININ_JOININCVARS_H
