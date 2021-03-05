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
#include "EditorDefs.h"

#include "VariableTypeInfo.h"
#include "TypeInfo_impl.h"
#include "IShader_info.h"

#ifndef AZ_MONOLITHIC_BUILD
#include "I3DEngine_info.h"
#endif

#include "Variable.h"
#include "UIEnumsDatabase.h"

// CryCommon
#include <CryTypeInfo.h>


IVariable* CVariableTypeInfo::Create(CTypeInfo::CVarInfo const& VarInfo, void* pAddress, const void* pAddressDefault)
{
    pAddress = VarInfo.GetAddress(pAddress);
    pAddressDefault = VarInfo.GetAddress(pAddressDefault);

    EType eType = GetType(VarInfo.Type);
    if (eType == ARRAY)
    {
        return new CVariableTypeInfoStruct(VarInfo, pAddress, pAddressDefault);
    }

    if (VarInfo.Type.EnumElem(0))
    {
        return new CVariableTypeInfoEnum(VarInfo, pAddress, pAddressDefault);
    }

    ISplineInterpolator* pSpline = 0;
    if (VarInfo.Type.ToValue(pAddress, pSpline))
    {
        return new CVariableTypeInfoSpline(VarInfo, pAddress, pAddressDefault, pSpline);
    }

    return new CVariableTypeInfo(VarInfo, pAddress, pAddressDefault, eType);
}

CVariableTypeInfo::EType CVariableTypeInfo::GetType(CTypeInfo const& typeInfo)
{
    // Translation to editor type values is currently done with some clunky type and name testing.
    if (typeInfo.HasSubVars())
    {
        if (typeInfo.IsType<Vec3>() && !typeInfo.NextSubVar(0)->Type.IsType<Vec3>())
        {
            // This is a vector type (and not a sub-classed vector type)
            return IVariable::VECTOR;
        }
        else
        {
            return IVariable::ARRAY;
        }
    }
    else if (typeInfo.IsType<bool>())
    {
        return IVariable::BOOL;
    }
    else if (typeInfo.IsType<int>() || typeInfo.IsType<uint>())
    {
        return IVariable::INT;
    }
    else if (typeInfo.IsType<float>())
    {
        return IVariable::FLOAT;
    }
    return IVariable::STRING;
}

CVariableTypeInfo::CVariableTypeInfo(CTypeInfo::CVarInfo const& VarInfo,
    void* pAddress, const void* pAddressDefault, EType eType)
    : m_pVarInfo(&VarInfo)
    , m_pData(pAddress)
    , m_pDefaultData(pAddressDefault)
{
    SetName(SpacedName(VarInfo.GetName()));
    SetTypes(VarInfo.Type, eType);
    SetFlags(IVariable::UI_UNSORTED | IVariable::UI_HIGHLIGHT_EDITED);
    SetDescription(VarInfo.GetComment());
}

void CVariableTypeInfo::SetTypes(CTypeInfo const& TypeInfo, EType eType)
{
    m_pTypeInfo = &TypeInfo;
    m_eType = eType;

    SetDataType(DT_SIMPLE);
    if (m_eType == VECTOR)
    {
        if (m_name == "Color")
        {
            SetDataType(DT_COLOR);
        }
    }
    else if (m_eType == STRING)
    {
        if (m_name == "Texture" || m_name == "Glow Map" || m_name == "Normal Map" || m_name == "Trail Fading")
        {
            SetDataType(DT_TEXTURE);
        }
        else if (m_name == "Material")
        {
            SetDataType(DT_MATERIAL);
        }
        else if (m_name == "Geometry")
        {
            SetDataType(DT_OBJECT);
        }
        else if (m_name == "Start Trigger" || m_name == "Stop Trigger")
        {
            SetDataType(DT_AUDIO_TRIGGER);
        }
        else if (m_name == "GeomCache")
        {
            SetDataType(DT_GEOM_CACHE);
        }
    }
}

// IVariable implementation.
CVariableTypeInfo::EType    CVariableTypeInfo::GetType() const
{
    return m_eType;
}

int CVariableTypeInfo::GetSize() const
{
    return m_pTypeInfo->Size;
}

void CVariableTypeInfo::GetLimits(float& fMin, float& fMax, float& fStep, bool& bHardMin, bool& bHardMax)
{
    // Get hard limits from variable type, or vector element type.
    const CTypeInfo* pLimitType = m_pTypeInfo;
    if ((m_eType == VECTOR || m_eType == VECTOR2) && m_pTypeInfo->NextSubVar(0))
    {
        pLimitType = &m_pTypeInfo->NextSubVar(0)->Type;
    }
    bHardMin = pLimitType->GetLimit(eLimit_Min, fMin);
    bHardMax = pLimitType->GetLimit(eLimit_Max, fMax);
    pLimitType->GetLimit(eLimit_Step, fStep);

    // Check var attrs for additional limits.
    if (m_pVarInfo->GetAttr("SoftMin", fMin))
    {
        bHardMin = false;
    }
    else if (m_pVarInfo->GetAttr("Min", fMin))
    {
        bHardMin = true;
    }

    if (m_pVarInfo->GetAttr("SoftMax", fMax))
    {
        bHardMax = false;
    }
    else if (m_pVarInfo->GetAttr("Max", fMax))
    {
        bHardMax = true;
    }
}

void CVariableTypeInfo::Set(const char* value)
{
    m_pTypeInfo->FromString(m_pData, value);
    OnSetValue(false);
}

void CVariableTypeInfo::Set(const QString& value)
{
    Set(value.toUtf8().data());
}

void CVariableTypeInfo::Set(float value)
{
    m_pTypeInfo->FromValue(m_pData, value);
    OnSetValue(false);
}

void CVariableTypeInfo::Set(int value)
{
    m_pTypeInfo->FromValue(m_pData, value);
    OnSetValue(false);
}

void CVariableTypeInfo::Set(bool value)
{
    m_pTypeInfo->FromValue(m_pData, value);
    OnSetValue(false);
}

void CVariableTypeInfo::Set(const Vec2& value)
{
    m_pTypeInfo->FromValue(m_pData, value);
    OnSetValue(false);
}

void CVariableTypeInfo::Set(const Vec3& value)
{
    m_pTypeInfo->FromValue(m_pData, value);
    OnSetValue(false);
}

void CVariableTypeInfo::Get(QString& value) const
{
    value = (const char*)m_pTypeInfo->ToString(m_pData);
}

void CVariableTypeInfo::Get(float& value) const
{
    m_pTypeInfo->ToValue(m_pData, value);
}

void CVariableTypeInfo::Get(int& value) const
{
    m_pTypeInfo->ToValue(m_pData, value);
}

void CVariableTypeInfo::Get(bool& value) const
{
    m_pTypeInfo->ToValue(m_pData, value);
}

void CVariableTypeInfo::Get(Vec2& value) const
{
    m_pTypeInfo->ToValue(m_pData, value);
}

void CVariableTypeInfo::Get(Vec3& value) const
{
    m_pTypeInfo->ToValue(m_pData, value);
}

bool CVariableTypeInfo::HasDefaultValue() const
{
    return m_pTypeInfo->ValueEqual(m_pData, m_pDefaultData);
}

void CVariableTypeInfo::ResetToDefault()
{
    QString strVal = m_pTypeInfo->ToString(m_pDefaultData).c_str();
    Set(strVal);
}

IVariable* CVariableTypeInfo::Clone([[maybe_unused]] bool bRecursive) const
{
    // Simply use a string var for universal conversion.
    IVariable* pClone = new CVariable<QString>();
    QString str;
    Get(str);
    pClone->Set(str);

    //add extra information for the clone: Name, DataType
    pClone->SetName(GetName());
    pClone->SetDataType(GetDataType());
    //use UserData to save eType since String Variable's GetType always return STRING
    pClone->SetUserData(GetType());

    return pClone;
}

void CVariableTypeInfo::CopyValue(IVariable* fromVar)
{
    assert(fromVar);
    QString str;
    fromVar->Get(str);
    Set(str);
}

CVariableTypeInfoEnum::CTypeInfoEnumList::CTypeInfoEnumList(CTypeInfo const& info)
    : TypeInfo(info)
{
}

QString CVariableTypeInfoEnum::CTypeInfoEnumList::GetItemName(uint index)
{
    return TypeInfo.EnumElem(index);
}

CVariableTypeInfoEnum::CVariableTypeInfoEnum(CTypeInfo::CVarInfo const& VarInfo,
    void* pAddress, const void* pAddressDefault, IVarEnumList* pEnumList)
    : CVariableTypeInfo(VarInfo, pAddress, pAddressDefault, UNKNOWN)
{
    // Use custom enum, or enum defined in TypeInfo.
    m_enumList = pEnumList ? pEnumList : new CTypeInfoEnumList(VarInfo.Type);
}

IVarEnumList* CVariableTypeInfoEnum::GetEnumList() const
{
    return m_enumList;
}

CVariableTypeInfoSpline::CVariableTypeInfoSpline(CTypeInfo::CVarInfo const& VarInfo,
    void* pAddress, const void* pAddressDefault, ISplineInterpolator* pSpline)
    : CVariableTypeInfo(VarInfo, pAddress, pAddressDefault, STRING)
    , m_pSpline(pSpline)
{
    if (m_pSpline && m_pSpline->GetNumDimensions() == 3)
    {
        SetDataType(DT_CURVE | DT_COLOR);
    }
    else
    {
        SetDataType(DT_CURVE | DT_PERCENT);
    }
}

CVariableTypeInfoSpline::~CVariableTypeInfoSpline()
{
    delete m_pSpline;
}

ISplineInterpolator* CVariableTypeInfoSpline::GetSpline()
{
    //if m_pSpline wasn't created or the data used to create spline was changed, we need create the m_pSpline
    int flags = GetFlags();
    if (m_pSpline == nullptr || flags & UI_CREATE_SPLINE)
    {
        if (m_pSpline != nullptr)
        {
            delete m_pSpline;
            m_pSpline = nullptr;
        }
        m_pTypeInfo->ToValue(m_pData, m_pSpline);
        flags &= ~UI_CREATE_SPLINE;
        SetFlags(flags);
    }
    return m_pSpline;
}

void CVariableTypeInfoSpline::OnSetValue(bool bRecursive)
{
    m_pTypeInfo->ToValue(m_pData, m_pSpline);
    CVariableTypeInfo::OnSetValue(bRecursive);
}

CVariableTypeInfoStruct::CVariableTypeInfoStruct(CTypeInfo::CVarInfo const& VarInfo,
    void* pAddress, const void* pAddressDefault)
    : CVariableTypeInfo(VarInfo, pAddress, pAddressDefault, ARRAY)
{
    ProcessSubStruct(VarInfo, pAddress, pAddressDefault);
}

void CVariableTypeInfoStruct::ProcessSubStruct(CTypeInfo::CVarInfo const& VarInfo, void* pAddress, const void* pAddressDefault)
{
    for AllSubVars(pSubVar, VarInfo.Type)
    {
        if (!*pSubVar->GetName())
        {
            EType eType = GetType(pSubVar->Type);
            if (eType == ARRAY)
            {
                // Recursively process nameless or base struct.
                ProcessSubStruct(*pSubVar, pSubVar->GetAddress(pAddress), pSubVar->GetAddress(pAddressDefault));
            }
            else if (pSubVar == VarInfo.Type.NextSubVar(0))
            {
                // Inline edit first sub-var in main field.
                SetTypes(pSubVar->Type, eType);
            }
        }
        else
        {
            IVariable* pVar = CVariableTypeInfo::Create(*pSubVar, pAddress, pAddressDefault);
            m_Vars.push_back(pVar);
        }
    }
}

QString CVariableTypeInfoStruct::GetDisplayValue() const
{
    return (const char*)m_pTypeInfo->ToString(m_pData);
}

void CVariableTypeInfoStruct::OnSetValue(bool bRecursive)
{
    CVariableBase::OnSetValue(bRecursive);
    if (bRecursive)
    {
        for (Vars::iterator it = m_Vars.begin(); it != m_Vars.end(); ++it)
        {
            (*it)->OnSetValue(true);
        }
    }
}

void CVariableTypeInfoStruct::SetFlagRecursive(EFlags flag)
{
    CVariableBase::SetFlagRecursive(flag);
    for (Vars::iterator it = m_Vars.begin(); it != m_Vars.end(); ++it)
    {
        (*it)->SetFlagRecursive(flag);
    }
}

void CVariableTypeInfoStruct::CopyValue(IVariable* fromVar)
{
    assert(fromVar);
    if (fromVar->GetType() != IVariable::ARRAY)
    {
        CVariableTypeInfo::CopyValue(fromVar);
    }

    int numSrc = fromVar->GetNumVariables();
    int numTrg = m_Vars.size();
    for (int i = 0; i < numSrc && i < numTrg; i++)
    {
        // Copy Every child variable.
        m_Vars[i]->CopyValue(fromVar->GetVariable(i));
    }
}

int CVariableTypeInfoStruct::GetNumVariables() const
{
    return m_Vars.size();
}

IVariable* CVariableTypeInfoStruct::GetVariable(int index) const
{
    return m_Vars[index];
}

CUIEnumsDBList::CUIEnumsDBList(CUIEnumsDatabase_SEnum const* pEnumList)
    : m_pEnumList(pEnumList)
{
}

QString CUIEnumsDBList::GetItemName(uint index)
{
    if (index >= m_pEnumList->strings.size())
    {
        return NULL;
    }
    return m_pEnumList->strings[index];
}
