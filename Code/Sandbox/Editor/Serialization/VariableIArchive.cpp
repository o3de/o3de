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

#include "EditorDefs.h"

#include "VariableIArchive.h"

// Editor
#include "Serialization/Decorators/Resources.h"
#include "Serialization/Decorators/Range.h"


using Serialization::CVariableIArchive;

namespace VarUtil
{
    _smart_ptr< IVariable > FindChildVariable(const _smart_ptr< IVariable >& pParent, const int childIndexOverride, const char* const name)
    {
        if (0 <= childIndexOverride)
        {
            return pParent->GetVariable(childIndexOverride);
        }
        else
        {
            const bool shouldSearchRecursively = false;
            return pParent->FindVariable(name, shouldSearchRecursively);
        }
    }


    template< typename T, typename TOut >
    bool ReadChildVariableAs(const _smart_ptr< IVariable >& pParent, const int childIndexOverride, const char* const name, TOut& valueOut)
    {
        _smart_ptr< IVariable > pVariable = FindChildVariable(pParent, childIndexOverride, name);
        if (pVariable)
        {
            T tmp;
            pVariable->Get(tmp);
            valueOut = static_cast< TOut >(tmp);
            return true;
        }
        return false;
    }


    template< typename T >
    bool ReadChildVariable(const _smart_ptr< IVariable >& pParent, const int childIndexOverride, const char* const name, T& valueOut)
    {
        return ReadChildVariableAs< T >(pParent, childIndexOverride, name, valueOut);
    }
}



CVariableIArchive::CVariableIArchive(const _smart_ptr< IVariable >& pVariable)
    : IArchive(IArchive::INPUT | IArchive::EDIT | IArchive::NO_EMPTY_NAMES)
    , m_pVariable(pVariable)
    , m_childIndexOverride(-1)
{
    CRY_ASSERT(m_pVariable);

    m_structHandlers[ TypeID::get < Serialization::IResourceSelector > ().name() ] = &CVariableIArchive::SerializeResourceSelector;
    m_structHandlers[ TypeID::get < Serialization::RangeDecorator < float >> ().name() ] = &CVariableIArchive::SerializeRangeFloat;
    m_structHandlers[ TypeID::get < Serialization::RangeDecorator < int >> ().name() ] = &CVariableIArchive::SerializeRangeInt;
    m_structHandlers[ TypeID::get < Serialization::RangeDecorator < unsigned int >> ().name() ] = &CVariableIArchive::SerializeRangeUInt;
    m_structHandlers[ TypeID::get < StringListStaticValue > ().name() ] = &CVariableIArchive::SerializeStringListStaticValue;
}


CVariableIArchive::~CVariableIArchive()
{
}


bool CVariableIArchive::operator()(bool& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< bool >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(Serialization::IString& value, const char* name, [[maybe_unused]] const char* label)
{
    QString stringValue;
    const bool readSuccess = VarUtil::ReadChildVariableAs< QString >(m_pVariable, m_childIndexOverride, name, stringValue);
    if (readSuccess)
    {
        value.set(stringValue.toUtf8().data());
        return true;
    }
    return false;
}


bool CVariableIArchive::operator()([[maybe_unused]] Serialization::IWString& value, [[maybe_unused]] const char* name, [[maybe_unused]] const char* label)
{
    CryFatalError("CVariableIArchive::operator() with IWString is not implemented");
    return false;
}


bool CVariableIArchive::operator()(float& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< float >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(double& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< float >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(int16& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(uint16& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(int32& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(uint32& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(int64& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(uint64& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(int8& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(uint8& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(char& value, const char* name, [[maybe_unused]] const char* label)
{
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, value);
}


bool CVariableIArchive::operator()(const Serialization::SStruct& ser, const char* name, const char* label)
{
    const char* const typeName = ser.type().name();
    HandlersMap::const_iterator it = m_structHandlers.find(typeName);
    const bool handlerFound = (it != m_structHandlers.end());
    if (handlerFound)
    {
        StructHandlerFunctionPtr pHandler = it->second;
        return (this->*pHandler)(ser, name, label);
    }

    return SerializeStruct(ser, name, label);
}


bool CVariableIArchive::operator()(Serialization::IContainer& ser, const char* name, [[maybe_unused]] const char* label)
{
    _smart_ptr< IVariable > pChild = VarUtil::FindChildVariable(m_pVariable, m_childIndexOverride, name);
    if (pChild)
    {
        const int elementCount = pChild->GetNumVariables();
        ser.resize(elementCount);

        if (0 < elementCount)
        {
            CVariableIArchive childArchive(pChild);
            childArchive.SetFilter(GetFilter());
            childArchive.SetInnerContext(GetInnerContext());

            for (int i = 0; i < elementCount; ++i)
            {
                childArchive.m_childIndexOverride = i;

                ser(childArchive, "", "");
                ser.next();
            }
        }
        return true;
    }
    return false;
}


bool CVariableIArchive::SerializeStruct(const Serialization::SStruct& ser, const char* name, [[maybe_unused]] const char* label)
{
    _smart_ptr< IVariable > pChild = VarUtil::FindChildVariable(m_pVariable, m_childIndexOverride, name);
    if (pChild)
    {
        CVariableIArchive childArchive(pChild);
        childArchive.SetFilter(GetFilter());
        childArchive.SetInnerContext(GetInnerContext());

        ser(childArchive);
        return true;
    }
    return false;
}

bool CVariableIArchive::SerializeResourceSelector(const Serialization::SStruct& ser, const char* name, [[maybe_unused]] const char* label)
{
    Serialization::IResourceSelector* pSelector = reinterpret_cast< Serialization::IResourceSelector* >(ser.pointer());

    QString stringValue;
    const bool readSuccess = VarUtil::ReadChildVariableAs< QString >(m_pVariable, m_childIndexOverride, name, stringValue);
    if (readSuccess)
    {
        pSelector->SetValue(stringValue.toUtf8().data());
        return true;
    }
    return false;
}


bool CVariableIArchive::SerializeStringListStaticValue(const Serialization::SStruct& ser, const char* name, [[maybe_unused]] const char* label)
{
    StringListStaticValue* const pStringListStaticValue = reinterpret_cast< StringListStaticValue* >(ser.pointer());
    const StringListStatic& stringListStatic = pStringListStaticValue->stringList();

    _smart_ptr< IVariable > pChild = VarUtil::FindChildVariable(m_pVariable, m_childIndexOverride, name);
    if (pChild)
    {
        int index = -1;
        pChild->Get(index);
        *pStringListStaticValue = index;
        return true;
    }
    return false;
}


bool CVariableIArchive::SerializeRangeFloat(const Serialization::SStruct& ser, const char* name, [[maybe_unused]] const char* label)
{
    const Serialization::RangeDecorator< float >* const pRange = reinterpret_cast< Serialization::RangeDecorator< float >* >(ser.pointer());
    return VarUtil::ReadChildVariableAs< float >(m_pVariable, m_childIndexOverride, name, *pRange->value);
}


bool CVariableIArchive::SerializeRangeInt(const Serialization::SStruct& ser, const char* name, [[maybe_unused]] const char* label)
{
    const Serialization::RangeDecorator< int >* const pRange = reinterpret_cast< Serialization::RangeDecorator< int >* >(ser.pointer());
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, *pRange->value);
}

bool CVariableIArchive::SerializeRangeUInt(const Serialization::SStruct& ser, const char* name, [[maybe_unused]] const char* label)
{
    const Serialization::RangeDecorator< unsigned int >* const pRange = reinterpret_cast< Serialization::RangeDecorator< unsigned int >* >(ser.pointer());
    return VarUtil::ReadChildVariableAs< int >(m_pVariable, m_childIndexOverride, name, *pRange->value);
}
