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

#ifndef CRYINCLUDE_EDITOR_UTIL_VARIABLETYPEINFO_H
#define CRYINCLUDE_EDITOR_UTIL_VARIABLETYPEINFO_H
#pragma once

#include "Variable.h"
#include "UIEnumsDatabase.h"
#include "VariableTypeInfo.h"

#include <CryTypeInfo.h>

//////////////////////////////////////////////////////////////////////////
// Adaptors for TypeInfo-defined variables to IVariable

inline QString SpacedName(const char* sName)
{
    // Split name with spaces.
    QString sSpacedName = sName;
    for (int i = 1; i < sSpacedName.length(); i++)
    {
        if (sSpacedName[i].isUpper() && sSpacedName[i - 1].isLower())
        {
            sSpacedName.insert(i, ' ');
            i++;
        }
    }
    return sSpacedName;
}

//////////////////////////////////////////////////////////////////////////
// Scalar variable
//////////////////////////////////////////////////////////////////////////

class EDITOR_CORE_API CVariableTypeInfo
    : public CVariableBase
{
public:
    // Dynamic constructor function
    static IVariable* Create(CTypeInfo::CVarInfo const& VarInfo, void* pBaseAddress, const void* pBaseAddressDefault);

    static EType GetType(CTypeInfo const& TypeInfo);

    CVariableTypeInfo(CTypeInfo::CVarInfo const& VarInfo, void* pAddress, const void* pAddressDefault, EType eType);

    void SetTypes(CTypeInfo const& TypeInfo, EType eType);

    // IVariable implementation.
    virtual EType   GetType() const;
    virtual int     GetSize() const;

    virtual void GetLimits(float& fMin, float& fMax, float& fStep, bool& bHardMin, bool& bHardMax);

    //////////////////////////////////////////////////////////////////////////
    // Access operators.
    //////////////////////////////////////////////////////////////////////////

    virtual void Set(const char* value);

    virtual void Set(const QString& value);

    virtual void Set(float value);

    virtual void Set(int value);

    virtual void Set(bool value);

    virtual void Set(const Vec2& value);

    virtual void Set(const Vec3& value);

    virtual void Get(QString& value) const;

    virtual void Get(float& value) const;

    virtual void Get(int& value) const;

    virtual void Get(bool& value) const;

    virtual void Get(Vec2& value) const;

    virtual void Get(Vec3& value) const;

    virtual bool HasDefaultValue() const;

    virtual void ResetToDefault();

    virtual IVariable* Clone(bool bRecursive) const;

    // To do: This could be more efficient ?
    virtual void CopyValue(IVariable* fromVar);

protected:
    CTypeInfo::CVarInfo const* m_pVarInfo;
    CTypeInfo const* m_pTypeInfo;           // TypeInfo system structure for this var.
    void* m_pData;                          // Existing address in memory. Directly modified.
    const void* m_pDefaultData;             // Address of default data for this var.
    EType m_eType;                          // Type info for editor.
    IEditor* m_pEditor;
};

//////////////////////////////////////////////////////////////////////////
// Enum variable
//////////////////////////////////////////////////////////////////////////

class EDITOR_CORE_API CVariableTypeInfoEnum
    : public CVariableTypeInfo
{
    struct CTypeInfoEnumList
        : IVarEnumList
    {
        CTypeInfo const& TypeInfo;

        CTypeInfoEnumList(CTypeInfo const& info);

        virtual QString GetItemName(uint index);
    };

public:

    // Constructor.
    CVariableTypeInfoEnum(CTypeInfo::CVarInfo const& VarInfo, void* pAddress, const void* pAddressDefault, IVarEnumList* pEnumList = 0);

    //////////////////////////////////////////////////////////////////////////
    // Additional IVariable implementation.
    //////////////////////////////////////////////////////////////////////////

    IVarEnumList* GetEnumList() const;

protected:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    TSmartPtr<IVarEnumList> m_enumList;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

//////////////////////////////////////////////////////////////////////////
// Spline variable
//////////////////////////////////////////////////////////////////////////

class EDITOR_CORE_API CVariableTypeInfoSpline
    : public CVariableTypeInfo
{

public:

    // Constructor.
    CVariableTypeInfoSpline(CTypeInfo::CVarInfo const& VarInfo, void* pAddress, const void* pAddressDefault, ISplineInterpolator* pSpline);

    ~CVariableTypeInfoSpline();

    virtual ISplineInterpolator* GetSpline();

    //! Overrides CVariableTypeInfo to keep m_pSpline in sync with CVariableTypeInfo::m_pData
    //! when Set(value) functions are called
    void OnSetValue(bool bRecursive) override;

private:
    ISplineInterpolator* m_pSpline;
};

//////////////////////////////////////////////////////////////////////////
// Struct variable
// Inherits implementation from CVariableArray
//////////////////////////////////////////////////////////////////////////

class EDITOR_CORE_API CVariableTypeInfoStruct
    : public CVariableTypeInfo
{
public:
    // Constructor.
    CVariableTypeInfoStruct(CTypeInfo::CVarInfo const& VarInfo, void* pAddress, const void* pAddressDefault);

    void ProcessSubStruct(CTypeInfo::CVarInfo const& VarInfo, void* pAddress, const void* pAddressDefault);

    //////////////////////////////////////////////////////////////////////////
    // IVariable implementation.
    //////////////////////////////////////////////////////////////////////////

    virtual QString GetDisplayValue() const;

    virtual void OnSetValue(bool bRecursive);

    void SetFlagRecursive(EFlags flag) override;

    void CopyValue(IVariable* fromVar);

    int GetNumVariables() const;

    IVariable* GetVariable(int index) const;

protected:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    typedef std::vector<IVariablePtr> Vars;
    Vars    m_Vars;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
struct EDITOR_CORE_API CUIEnumsDBList
    : IVarEnumList
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    CUIEnumsDatabase_SEnum const* m_pEnumList;

    CUIEnumsDBList(CUIEnumsDatabase_SEnum const* pEnumList);

    virtual QString GetItemName(uint index);
};

#endif // CRYINCLUDE_EDITOR_UTIL_VARIABLETYPEINFO_H
