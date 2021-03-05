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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_INTRUSIVEFACTORY_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_INTRUSIVEFACTORY_H
#pragma once

#include <StlUtils.h>

template<class TBase>
class CIntrusiveFactory
{
private:
    struct ICreator
    {
        virtual TBase* Create() const = 0;
    };

public:
    template<class TDerived>
    struct SCreator
        : ICreator
    {
        SCreator() { CIntrusiveFactory<TBase>::Instance().RegisterType<TDerived>(this); }

        TBase* Create() const override { return new TDerived(); }
    };

    static CIntrusiveFactory& Instance() { static CIntrusiveFactory instance; return instance; }

    TBase* Create(const char* keyType) const
    {
        TCreatorByType::const_iterator it = m_creators.find(keyType);
        if (it == m_creators.end() || it->second == 0)
        {
            return 0;
        }
        else
        {
            return it->second->Create();
        }
    }

    struct SSerializer
    {
        _smart_ptr<TBase>& pointer;

        SSerializer(_smart_ptr<TBase>& pointer)
            : pointer(pointer) {}

        void Serialize(Serialization::IArchive& ar);
    };

private:
    template<class TDerived>
    void RegisterType(ICreator* creator)
    {
        const char* type = TDerived::GetType();
        m_creators[type] = creator;
    }

    typedef std::map<const char*, ICreator*, stl::less_strcmp<const char*> > TCreatorByType;
    TCreatorByType m_creators;
};

template<class TBase>
void CIntrusiveFactory<TBase>::SSerializer::Serialize(Serialization::IArchive & ar)
{
    string type = pointer.get() ? pointer->GetInstanceType() : "";
    string oldType = type;
    ar(type, "type", "Type");
    if (ar.IsInput())
    {
        if (oldType != type)
        {
            pointer.reset(CIntrusiveFactory<TBase>::Instance().Create(type.c_str()));
        }
    }
    if (pointer)
    {
        pointer->Serialize(ar);
    }
}



#define REGISTER_IN_INTRUSIVE_FACTORY(BaseType, DerivedType) namespace { CIntrusiveFactory<BaseType>::SCreator<DerivedType> baseType##DerivedType##_Creator; }

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_INTRUSIVEFACTORY_H
