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

#pragma once

#include <Serialization/Serializer.h>

#include "ClassFactory.h"

template <class T>
class BoostSharedPtrSerializer
    : public Serialization::IPointer
{
public:
    BoostSharedPtrSerializer(AZStd::shared_ptr<T>& ptr)
        : m_ptr(ptr)
    {
    }

    const char* registeredTypeName() const override
    {
        if (m_ptr)
        {
            return factoryOverride().getRegisteredTypeName(m_ptr.get());
        }
        else
        {
            return "";
        }
    }

    void create(const char* registeredTypeName) const override
    {
        CRY_ASSERT(!m_ptr || m_ptr.use_count() == 1);
        if (registeredTypeName && registeredTypeName[0] != '\0')
        {
            m_ptr.reset(factoryOverride().create(registeredTypeName));
        }
        else
        {
            m_ptr.reset();
        }
    }

    Serialization::TypeID baseType() const override
    {
        return Serialization::TypeID::get<T>();
    }

    virtual Serialization::SStruct serializer() const override
    {
        return Serialization::SStruct(*m_ptr);
    }

    void* get() const
    {
        return reinterpret_cast<void*>(m_ptr.get());
    }

    const void* handle() const
    {
        return &m_ptr;
    }

    Serialization::TypeID pointerType() const override
    {
        return Serialization::TypeID::get<AZStd::shared_ptr<T> >();
    }

    Serialization::ClassFactory<T>* factory() const override
    {
        return &factoryOverride();
    }

    virtual Serialization::ClassFactory<T>& factoryOverride() const
    {
        return Serialization::ClassFactory<T>::the();
    }

protected:
    AZStd::shared_ptr<T>& m_ptr;
};

namespace AZStd
{
    template <class T>
    bool Serialize(Serialization::IArchive& ar, AZStd::shared_ptr<T>& ptr, const char* name, const char* label)
    {
        BoostSharedPtrSerializer<T> serializer(ptr);
        return ar(static_cast<Serialization::IPointer&>(serializer), name, label);
    }
}
