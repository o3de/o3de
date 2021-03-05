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

// Description : Implementation of the IProfileLogSystem interface, which is used to
//               save hierarchical log with SHierProfileLogItem


#ifndef CRYINCLUDE_CRYSYSTEM_PROFILELOGSYSTEM_H
#define CRYINCLUDE_CRYSYSTEM_PROFILELOGSYSTEM_H

#pragma once

#include "ProfileLog.h"

class CLogElement
    : public ILogElement
{
public:
    CLogElement();
    CLogElement(CLogElement* pParent);
    CLogElement(CLogElement* pParent, const char* name, const char* message);

    virtual ILogElement*    Log         (const char* name, const char* message);
    virtual ILogElement*    SetTime (float time);
    virtual void                    Flush       (stack_string& indent);

    void                                    Clear       ();

    inline void SetName(const char* name)
    {
        m_strName = name;
    }

    inline void SetMessage(const char* message)
    {
        m_strMessage = message;
    }

private:
    string m_strName;
    string m_strMessage;
    float    m_time;            // milliSeconds

    CLogElement* m_pParent;
    std::list<CLogElement> m_logElements;
};

class CProfileLogSystem
    : public IProfileLogSystem
{
public:
    CProfileLogSystem();
    ~CProfileLogSystem();

    virtual ILogElement*    Log         (const char* name, const char* message);
    virtual void                    SetTime (ILogElement* pElement, float time);
    virtual void                    Release ();

private:
    CLogElement  m_rootElelent;
    ILogElement* m_pLastElelent;
};

#endif // CRYINCLUDE_CRYSYSTEM_PROFILELOGSYSTEM_H
