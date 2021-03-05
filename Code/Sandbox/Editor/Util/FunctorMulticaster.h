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

// Description : Multicast multiple functors.


#ifndef CRYINCLUDE_EDITOR_UTIL_FUNCTORMULTICASTER_H
#define CRYINCLUDE_EDITOR_UTIL_FUNCTORMULTICASTER_H
#pragma once


//////////////////////////////////////////////////////////////////////////
template <class _event>
class FunctorMulticaster
{
public:
    typedef Functor1<_event> callback;

    FunctorMulticaster() {}
    virtual ~FunctorMulticaster() {}

    void AddListener(const callback& func)
    {
        m_listeners.push_back(func);
    }

    void RemoveListener(const callback& func)
    {
        m_listeners.remove(func);
    }

    void Call(_event evt)
    {
        typename std::list<callback>::iterator iter;
        for (iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
        {
            (*iter)(evt);
        }
    }

private:
    std::list<callback> m_listeners;
};

#endif // CRYINCLUDE_EDITOR_UTIL_FUNCTORMULTICASTER_H
