/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_UTILS_REFLECTEDVARWRAPPER_H
#define CRYINCLUDE_EDITOR_UTILS_REFLECTEDVARWRAPPER_H
#pragma once

#include "Util/Variable.h"
#include <Util/VariablePropertyType.h>
#include "ReflectedVar.h"

#include <QScopedPointer>

struct CUIEnumsDatabase_SEnum;
class ReflectedPropertyItem;

// Class to wrap the CReflectedVars and sync them with corresponding IVariable.
// Most of this code is ported from CPropertyItem functions that marshal data between
// IVariable and editor widgets.

class EDITOR_CORE_API ReflectedVarAdapter
{
public:
    virtual ~ReflectedVarAdapter(){};

    // update the range limits in CReflectedVar to range specified in IVariable
    virtual void UpdateRangeLimits([[maybe_unused]] IVariable* pVariable) {};

    //set IVariable for this property and create a CReflectedVar to represent it
    virtual void SetVariable(IVariable* pVariable) = 0;

    //update the ReflectedVar to current value of IVar
    virtual void SyncReflectedVarToIVar(IVariable* pVariable) = 0;

    //update the internal IVariable as result of ReflectedVar changing
    virtual void SyncIVarToReflectedVar(IVariable* pVariable) = 0;

    // Callback called when variable change. SyncReflectedVarToIVar will be called after
    virtual void OnVariableChange([[maybe_unused]] IVariable* var) {};

    virtual bool UpdateReflectedVarEnums() { return false; }

    virtual CReflectedVar* GetReflectedVar() = 0;

    //needed for containers that can have new values filled in
    virtual void ReplaceVarBlock([[maybe_unused]] CVarBlock* varBlock) {};

    virtual bool Contains(CReflectedVar* var) { return GetReflectedVar() == var; }
};


class EDITOR_CORE_API ReflectedVarIntAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void UpdateRangeLimits(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarInt > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    float m_valueMultiplier = 1.0f;
};

class EDITOR_CORE_API ReflectedVarFloatAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void UpdateRangeLimits(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarFloat > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    float m_valueMultiplier = 1.0f;
};

class EDITOR_CORE_API ReflectedVarStringAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarString > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

class EDITOR_CORE_API ReflectedVarBoolAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarBool > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

class EDITOR_CORE_API ReflectedVarEnumAdapter
    : public ReflectedVarAdapter
{
public:
    ReflectedVarEnumAdapter();
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    void OnVariableChange(IVariable* var) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }

protected:
    //update the ReflectedVar with the allowable enum options
    bool UpdateReflectedVarEnums() override;
    
    //virtual function to allow derived classes to update the enum list before syncing with ReflectedVar.
    virtual void updateIVariableEnumList([[maybe_unused]] IVariable* pVariable) {};

private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarEnum<AZStd::string>  > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    IVariable* m_pVariable;
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    IVarEnumListPtr m_enumList;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    bool m_updatingEnums;
};

class EDITOR_CORE_API ReflectedVarDBEnumAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarEnum<AZStd::string>  > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    CUIEnumsDatabase_SEnum* m_pEnumDBItem;
};

class EDITOR_CORE_API ReflectedVarVector2Adapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarVector2 > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

class EDITOR_CORE_API ReflectedVarVector3Adapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarVector3 > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

class EDITOR_CORE_API ReflectedVarVector4Adapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarVector4 > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};


class EDITOR_CORE_API ReflectedVarColorAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarColor > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

class EDITOR_CORE_API ReflectedVarResourceAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarResource> m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

class EDITOR_CORE_API ReflectedVarUserAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable *pVariable) override;
    void SyncReflectedVarToIVar(IVariable *pVariable) override;
    void SyncIVarToReflectedVar(IVariable *pVariable) override;
    CReflectedVar *GetReflectedVar() override {
        return m_reflectedVar.data();
    }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarUser> m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

class EDITOR_CORE_API ReflectedVarSplineAdapter
    : public ReflectedVarAdapter
{
public:
    ReflectedVarSplineAdapter(ReflectedPropertyItem *parentItem, PropertyType propertyType);
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override {
        return m_reflectedVar.data();
    }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarSpline > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    bool m_bDontSendToControl;
    PropertyType m_propertyType;
    ReflectedPropertyItem *m_parentItem;
};


class EDITOR_CORE_API ReflectedVarGenericPropertyAdapter
    : public ReflectedVarAdapter
{
public:
    ReflectedVarGenericPropertyAdapter(PropertyType propertyType);
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarGenericProperty > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    PropertyType m_propertyType;
};

class EDITOR_CORE_API ReflectedVarMotionAdapter
    : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }
private:
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<CReflectedVarMotion > m_reflectedVar;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};


#endif // CRYINCLUDE_EDITOR_UTILS_REFLECTEDVARWRAPPER_H
