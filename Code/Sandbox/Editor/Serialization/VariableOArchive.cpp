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

#include "VariableOArchive.h"

// Editor
#include "Serialization/Decorators/Resources.h"
#include "Serialization/Decorators/Range.h"


using Serialization::CVariableOArchive;

namespace VarUtil
{
    template< typename T >
    _smart_ptr< IVariable > AddChildVariable(const _smart_ptr< IVariable >& pVariableArray, const T& value, const char* const name, const char* label)
    {
        CRY_ASSERT(pVariableArray);

        _smart_ptr< IVariable > pVariable = new CVariable< T >();
        pVariable->SetName(name);
        pVariable->SetHumanName(label);
        pVariable->Set(value);

        pVariableArray->AddVariable(pVariable);

        return pVariable;
    }

    template< typename TMin, typename TMax >
    void SetLimits(const _smart_ptr< IVariable >& pVariable, const TMin minValue, const TMax maxValue)
    {
        pVariable->SetLimits(static_cast< float >(minValue), static_cast< float >(maxValue));
    }
}

CVariableOArchive::CVariableOArchive()
    : IArchive(IArchive::OUTPUT | IArchive::EDIT | IArchive::NO_EMPTY_NAMES)
    , m_pVariable(new CVariableArray())
{
    m_resourceHandlers[ "Animation" ] = &CVariableOArchive::SerializeAnimationName;
    m_resourceHandlers[ "Sound" ] = &CVariableOArchive::SerializeSoundName;
    m_resourceHandlers[ "Model" ] = &CVariableOArchive::SerializeObjectFilename;

    m_structHandlers[ TypeID::get < Serialization::IResourceSelector > ().name() ] = &CVariableOArchive::SerializeIResourceSelector;
    m_structHandlers[ TypeID::get < Serialization::RangeDecorator < float >> ().name() ] = &CVariableOArchive::SerializeRangeFloat;
    m_structHandlers[ TypeID::get < Serialization::RangeDecorator < int >> ().name() ] = &CVariableOArchive::SerializeRangeInt;
    m_structHandlers[ TypeID::get < Serialization::RangeDecorator < unsigned int >> ().name() ] = &CVariableOArchive::SerializeRangeUInt;
    m_structHandlers[ TypeID::get < StringListStaticValue > ().name() ] = &CVariableOArchive::SerializeStringListStaticValue;
}


CVariableOArchive::~CVariableOArchive()
{
}


_smart_ptr< IVariable > CVariableOArchive::GetIVariable() const
{
    return m_pVariable;
}


CVarBlockPtr CVariableOArchive::GetVarBlock() const
{
    CVarBlockPtr pVarBlock = new CVarBlock();
    pVarBlock->AddVariable(m_pVariable);
    return pVarBlock;
}


bool CVariableOArchive::operator()(bool& value, const char* name, const char* label)
{
    VarUtil::AddChildVariable< bool >(m_pVariable, value, name, label);
    return true;
}


bool CVariableOArchive::operator()(Serialization::IString& value, const char* name, const char* label)
{
    const QString valueString = value.get();
    VarUtil::AddChildVariable< QString >(m_pVariable, valueString, name, label);
    return true;
}


bool CVariableOArchive::operator()([[maybe_unused]] Serialization::IWString& value, [[maybe_unused]] const char* name, [[maybe_unused]] const char* label)
{
    CryFatalError("CVarBlockOArchive::operator() with IWString is not implemented");
    return false;
}


bool CVariableOArchive::operator()(float& value, const char* name, const char* label)
{
    VarUtil::AddChildVariable< float >(m_pVariable, value, name, label);
    return true;
}


bool CVariableOArchive::operator()(double& value, const char* name, const char* label)
{
    VarUtil::AddChildVariable< float >(m_pVariable, value, name, label);
    return true;
}


bool CVariableOArchive::operator()(int16& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, SHRT_MIN, SHRT_MAX);
    return true;
}


bool CVariableOArchive::operator()(uint16& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, 0, USHRT_MAX);
    return true;
}


bool CVariableOArchive::operator()(int32& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, INT_MIN, INT_MAX);
    return true;
}


bool CVariableOArchive::operator()(uint32& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, 0, INT_MAX);
    return true;
}


bool CVariableOArchive::operator()(int64& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, INT_MIN, INT_MAX);
    return true;
}


bool CVariableOArchive::operator()(uint64& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, 0, INT_MAX);
    return true;
}


bool CVariableOArchive::operator()(int8& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, SCHAR_MIN, SCHAR_MAX);
    return true;
}


bool CVariableOArchive::operator()(uint8& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, 0, UCHAR_MAX);
    return true;
}


bool CVariableOArchive::operator()(char& value, const char* name, const char* label)
{
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, value, name, label);
    VarUtil::SetLimits(pVariable, CHAR_MIN, CHAR_MAX);
    return true;
}


bool CVariableOArchive::operator()(const Serialization::SStruct& ser, const char* name, const char* label)
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

static const char* gVec4Names[] = { "X", "Y", "Z", "W" };
static const char* gEmptyNames[] = { "" };

bool CVariableOArchive::operator()(Serialization::IContainer& ser, const char* name, const char* label)
{
    CVariableOArchive childArchive;
    childArchive.SetFilter(GetFilter());
    childArchive.SetInnerContext(GetInnerContext());

    _smart_ptr< IVariable > pChildVariable = childArchive.GetIVariable();
    pChildVariable->SetName(name);
    pChildVariable->SetHumanName(label);

    m_pVariable->AddVariable(pChildVariable);

    const size_t containerSize = ser.size();

    const char** nameArray = gEmptyNames;
    size_t nameArraySize = 1;
    if (containerSize >= 2 && containerSize <= 4)
    {
        nameArray = gVec4Names;
        nameArraySize = containerSize;
    }

    size_t index = 0;
    if (0 < containerSize)
    {
        do
        {
            ser(childArchive, nameArray[index % nameArraySize], nameArray[index % nameArraySize]);
            ++index;
        } while (ser.next());
    }

    return true;
}


bool CVariableOArchive::SerializeStruct(const Serialization::SStruct& ser, const char* name, const char* label)
{
    CVariableOArchive childArchive;
    childArchive.SetFilter(GetFilter());
    childArchive.SetInnerContext(GetInnerContext());

    _smart_ptr< IVariable > pChildVariable = childArchive.GetIVariable();
    pChildVariable->SetName(name);
    pChildVariable->SetHumanName(label);

    m_pVariable->AddVariable(pChildVariable);

    const bool serializeSuccess = ser(childArchive);

    return serializeSuccess;
}


bool CVariableOArchive::SerializeAnimationName(const Serialization::IResourceSelector* pSelector, const char* name, const char* label)
{
    const QString valueString = pSelector->GetValue();
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< QString >(m_pVariable, valueString, name, label);
    pVariable->SetDataType(IVariable::DT_ANIMATION);

    return true;
}


bool CVariableOArchive::SerializeSoundName(const Serialization::IResourceSelector* pSelector, const char* name, const char* label)
{
    const QString valueString = pSelector->GetValue();
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< QString >(m_pVariable, valueString, name, label);
    pVariable->SetDataType(IVariable::DT_AUDIO_TRIGGER);

    return true;
}

void CVariableOArchive::CreateChildEnumVariable(const QStringList& enumValues, const QString& value, const char* name, const char* label)
{
    if (enumValues.empty())
    {
        VarUtil::AddChildVariable< QString >(m_pVariable, value, name, label);
    }
    else
    {
        _smart_ptr< CVariableEnum< QString > > pVariable = new CVariableEnum< QString >();
        pVariable->SetName(name);
        pVariable->SetHumanName(label);

        pVariable->AddEnumItem("", "");

        const size_t enumValuesCount = enumValues.size();
        for (size_t i = 0; i < enumValuesCount; ++i)
        {
            pVariable->AddEnumItem(enumValues[ i ], enumValues[ i ]);
        }

        pVariable->Set(value);

        m_pVariable->AddVariable(pVariable);
    }
}

bool CVariableOArchive::SerializeObjectFilename(const Serialization::IResourceSelector* pSelector, const char* name, const char* label)
{
    const QString valueString = pSelector->GetValue();
    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< QString >(m_pVariable, valueString, name, label);
    pVariable->SetDataType(IVariable::DT_OBJECT);

    return true;
}


bool CVariableOArchive::SerializeStringListStaticValue(const Serialization::SStruct& ser, const char* name, const char* label)
{
    const StringListStaticValue* const pStringListStaticValue = reinterpret_cast< StringListStaticValue* >(ser.pointer());
    const StringListStatic& stringListStatic = pStringListStaticValue->stringList();
    const int index = pStringListStaticValue->index();

    _smart_ptr< CVariableEnum< int > > pVariable = new CVariableEnum< int >();
    pVariable->SetName(name);
    pVariable->SetHumanName(label);

    const size_t stringListStaticSize = stringListStatic.size();
    for (size_t i = 0; i < stringListStaticSize; ++i)
    {
        pVariable->AddEnumItem(stringListStatic[ i ], static_cast< int >(i));
    }

    if (0 <= index)
    {
        CRY_ASSERT(index < stringListStaticSize);
        pVariable->Set(static_cast< int >(index));
    }

    m_pVariable->AddVariable(pVariable);

    return true;
}

bool CVariableOArchive::SerializeIResourceSelector(const Serialization::SStruct& ser, const char* name, const char* label)
{
    const Serialization::IResourceSelector* pSelector = reinterpret_cast< Serialization::IResourceSelector* >(ser.pointer());

    ResourceHandlersMap::iterator it = m_resourceHandlers.find(pSelector->resourceType);
    if (it != m_resourceHandlers.end())
    {
        return (this->*(it->second))(pSelector, name, label);
    }
    return false;
}

template<class T>
static void SetLimits(IVariable* pVariable, const Serialization::RangeDecorator<T>* pRange, float stepValue)
{
    if (pRange->softMin != std::numeric_limits<T>::lowest() || pRange->softMax != std::numeric_limits<T>::max())
    {
        float minimal = (float)pRange->softMin;
        float maximal = (float)pRange->softMax;
        bool hardMin = false;
        bool hardMax = false;
        if (pRange->hardMin != std::numeric_limits<T>::lowest())
        {
            minimal = pRange->hardMin;
            hardMin = true;
        }
        if (pRange->hardMax != std::numeric_limits<T>::max())
        {
            maximal = pRange->hardMax;
            hardMax = true;
        }
        pVariable->SetLimits(minimal, maximal, stepValue, hardMin, hardMax);
    }
    else
    {
        float minimal = 0.0f;
        float maximal = 0.0f;
        float oldStep = 0.0f;
        bool hardMin = false;
        bool hardMax = false;
        pVariable->GetLimits(minimal, maximal, oldStep, hardMin, hardMax);
        pVariable->SetLimits(minimal, maximal, stepValue, hardMin, hardMax);
    }
}

bool CVariableOArchive::SerializeRangeFloat(const Serialization::SStruct& ser, const char* name, const char* label)
{
    const Serialization::RangeDecorator< float >* const pRange = reinterpret_cast< Serialization::RangeDecorator< float >* >(ser.pointer());

    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< float >(m_pVariable, *pRange->value, name, label);

    SetLimits(pVariable.get(), pRange, 0.01f);
    return true;
}

bool CVariableOArchive::SerializeRangeInt(const Serialization::SStruct& ser, const char* name, const char* label)
{
    const Serialization::RangeDecorator< int >* const pRange = reinterpret_cast< Serialization::RangeDecorator< int >* >(ser.pointer());

    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, *pRange->value, name, label);
    SetLimits(pVariable.get(), pRange, 1.0f);
    return true;
}

bool CVariableOArchive::SerializeRangeUInt(const Serialization::SStruct& ser, const char* name, const char* label)
{
    const Serialization::RangeDecorator< unsigned int >* const pRange = reinterpret_cast< Serialization::RangeDecorator< unsigned int >* >(ser.pointer());

    _smart_ptr< IVariable > pVariable = VarUtil::AddChildVariable< int >(m_pVariable, *pRange->value, name, label);
    SetLimits(pVariable, pRange, 1.0f);
    return true;
}
