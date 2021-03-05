/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.
#ifndef FUNCTIONTHREAD_H
#define FUNCTIONTHREAD_H

#include <QThread>

#if !defined(AZ_PLATFORM_WINDOWS)
#define STILL_ACTIVE 259
#endif

class FunctionThread : public QThread
{
public:
    FunctionThread(unsigned int (*function)(void* param), void* param)
    : m_function(function),
    m_param(param)
    {
    }

    unsigned int returnCode() const
    {
        return m_returnCode;
    }

    static FunctionThread* CreateThread(int,int, unsigned int (*function)(void* param), void* param, int, int)
    {
        auto t = new FunctionThread(function, param);
        t->start();
        return t;
    }

protected:
    void run() override
    {
        m_returnCode = m_function(m_param);
    }

private:
    unsigned int m_returnCode = STILL_ACTIVE;
    unsigned int (*m_function)(void* param);
    void* m_param;
};

#endif
