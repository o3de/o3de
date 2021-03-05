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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEINFO_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEINFO_H
#pragma once


namespace Serialization {
    class IArchive;
}

struct STypeInfoInstance
{
    template<class T>
    STypeInfoInstance(T& obj)
        : m_pTypeInfo(&TypeInfo(&obj))
        , m_pObject(&obj)
    {
    }

    STypeInfoInstance(const CTypeInfo* typeInfo, void* object)
        : m_pTypeInfo(typeInfo)
        , m_pObject(object)
    {
    }

    inline void Serialize(Serialization::IArchive& ar);

    const CTypeInfo* m_pTypeInfo;
    void* m_pObject;
    std::set<string> m_persistentStrings;
};

#include "TypeInfoImpl.h"
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEINFO_H
