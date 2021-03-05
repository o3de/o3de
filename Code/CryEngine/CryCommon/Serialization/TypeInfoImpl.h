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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEINFOIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEINFOIMPL_H
#pragma once



#include "CryTypeInfo.h"
#include "Serialization/Decorators/Range.h"
#include "Serialization/Enum.h"
#include "ISplines.h"
#include <StringUtils.h>
#include "Serialization/Color.h"
#include "Cry_Color.h"
#include "Decorators/Resources.h"

struct SPrivateTypeInfoInstanceLevel
{
    SPrivateTypeInfoInstanceLevel(const CTypeInfo* typeInfo, void* object, STypeInfoInstance* instance)
        : m_pTypeInfo(typeInfo)
        , m_pObject(object)
        , m_instance(instance)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        for AllSubVars(pVar, *m_pTypeInfo)
        {
            string group;
            if (pVar->GetAttr("Group", group))
            {
                if (!m_sCurrentGroup.empty())
                {
                    ar.CloseBlock();
                }
                const char* name = m_instance->m_persistentStrings.insert(group).first->c_str();
                ar.OpenBlock(name, name);
                m_sCurrentGroup = name;
            }
            else
            {
                const char* name = pVar->GetName();
                if (!*name)
                {
                    name = pVar->Type.Name;
                }

                const char* label = pVar->GetName();
                if (!*label)
                {
                    string n = "^";
                    n += pVar->GetName();
                    label = m_instance->m_persistentStrings.insert(n).first->c_str();
                }

                SerializeVariable(pVar, m_pObject, ar, name, label);
            }
        }

        if (!m_sCurrentGroup.empty())
        {
            ar.CloseBlock();
            m_sCurrentGroup.clear();
        }
    }

    template <class T>
    void SerializeT(const CTypeInfo::CVarInfo* pVar, void* pParentAddress, Serialization::IArchive& ar, const char* name, const char* label)
    {
        T value;
        const CTypeInfo& type = pVar->Type;
        type.ToValue(pVar->GetAddress(pParentAddress), value);
        ar(value, name, label);
        if (ar.IsInput())
        {
            type.FromValue(pVar->GetAddress(pParentAddress), value);
        }
    }

    template <class T>
    void SerializeNumericalT(const CTypeInfo::CVarInfo* pVar, void* pParentAddress, Serialization::IArchive& ar, const char* name, const char* label)
    {
        T value;
        const CTypeInfo& type = pVar->Type;
        type.ToValue(pVar->GetAddress(pParentAddress), value);

        float limMin, limMax;
        if (pVar->GetLimit(eLimit_Min, limMin) && pVar->GetLimit(eLimit_Max, limMax))
        {
            ar(Serialization::Range<T>(value, limMin, limMax), name, label);
        }
        else
        {
            ar(value, name, label);
        }
        if (ar.IsInput())
        {
            type.FromValue(pVar->GetAddress(pParentAddress), value);
        }
    }

    void SerializeVariable(const CTypeInfo::CVarInfo* pVar, void* pParentAddress, Serialization::IArchive& ar, const char* name, const char* label)
    {
        const CTypeInfo& type = pVar->Type;

        if (type.HasSubVars())
        {
            if (strcmp(name, "Color") == 0)
            {
                Color3F value;
                const CTypeInfo& type = pVar->Type;
                type.ToValue(pVar->GetAddress(pParentAddress), value);
                ColorF colour = value;
                ar(colour, name, label);
                if (ar.IsInput())
                {
                    value = Color3F(colour.r, colour.g, colour.b);
                    type.FromValue(pVar->GetAddress(pParentAddress), value);
                }
            }
            else
            {
                // load params of sub-variables (variable is a struct or vector)
                SPrivateTypeInfoInstanceLevel instance(&type, pVar->GetAddress(pParentAddress), m_instance);
                ar(instance, name, label);
            }
        }
        else
        {
            if (type.IsType<bool>())
            {
                SerializeT<bool>(pVar, pParentAddress, ar, name, label);
            }
            else if (type.IsType<unsigned char>())
            {
                SerializeNumericalT<unsigned char>(pVar, pParentAddress, ar, name, label);
            }
            else if (type.IsType<char>())
            {
                SerializeNumericalT<char>(pVar, pParentAddress, ar, name, label);
            }
            else if (type.IsType<int>())
            {
                SerializeNumericalT<int>(pVar, pParentAddress, ar, name, label);
            }
            else if (type.IsType<uint>())
            {
                SerializeNumericalT<uint>(pVar, pParentAddress, ar, name, label);
            }
            else if (type.IsType<float>())
            {
                SerializeNumericalT<float>(pVar, pParentAddress, ar, name, label);
            }
            else if (type.EnumElem(0))
            {
                Serialization::StringList stringList;
                const char* enumType = type.EnumElem(0);
                for (int i = 1; enumType; ++i)
                {
                    stringList.push_back(enumType);
                    enumType = type.EnumElem(i);
                }

                string enumValue = pVar->ToString(pParentAddress);
                int index = std::max(stringList.find(enumValue.c_str()), 0);

                Serialization::StringListValue stringListValue(stringList, index);
                ar(stringListValue, name, label);
                if (ar.IsInput())
                {
                    pVar->FromString(pParentAddress, stringListValue.c_str());
                }
            }
            else
            {
                ISplineInterpolator* pSpline = 0;
                if (type.ToValue(pVar->GetAddress(pParentAddress), pSpline))
                {
                    // TODO: Curve field
                }
                else
                {
                    string value;
                    value = pVar->ToString(pParentAddress);

                    if (strcmp(name, "Texture") == 0)
                    {
                        // TODO: Texture field
                    }
                    else if (strcmp(name, "Material") == 0)
                    {
                        // TODO: Material field
                    }
                    else if (strcmp(name, "Geometry") == 0)
                    {
                        ar(Serialization::ModelFilename(value), name, label);
                    }
                    else if (strcmp(name, "Sound") == 0)
                    {
                        ar(Serialization::SoundName(value), name, label);
                    }
                    else if (strcmp(name, "GeomCache") == 0)
                    {
                        // TODO: Geom cache field
                    }
                    else
                    {
                        ar(value, name, label);
                    }
                    if (ar.IsInput())
                    {
                        pVar->FromString(pParentAddress, value.c_str());
                    }
                }
            }
        }
    }

private:
    const CTypeInfo* m_pTypeInfo;
    void* m_pObject;
    string m_sCurrentGroup;
    STypeInfoInstance* m_instance;
};

//------------------------------
inline void STypeInfoInstance::Serialize(Serialization::IArchive& ar)
{
    SPrivateTypeInfoInstanceLevel instance(m_pTypeInfo, m_pObject, this);
    instance.Serialize(ar);
}
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEINFOIMPL_H
