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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYNAMEIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYNAMEIMPL_H
#pragma once

#include "CryName.h"
#include "IArchive.h"

class CryNameSerializer
    : public Serialization::IString
{
public:
    CryNameSerializer(CCryName& s)
        : m_s(s)
    {
    }

    virtual void set(const char* value)
    {
        m_s = value;
    }

    virtual const char* get() const
    {
        return m_s.c_str();
    }

    virtual const void* handle() const
    {
        return &m_s;
    }

    virtual Serialization::TypeID type() const
    {
        return Serialization::TypeID::get<CCryName>();
    }

    CCryName& m_s;
};


inline bool Serialize(Serialization::IArchive& ar, CCryName& cryName, const char* name, const char* label)
{
    CryNameSerializer serializer(cryName);
    return ar(static_cast<Serialization::IString&>(serializer), name, label);
}


#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYNAMEIMPL_H
