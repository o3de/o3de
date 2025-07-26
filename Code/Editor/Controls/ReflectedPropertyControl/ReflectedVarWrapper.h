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

    QScopedPointer<CReflectedVarInt > m_reflectedVar;
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
    QScopedPointer<CReflectedVarFloat > m_reflectedVar;
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
    QScopedPointer<CReflectedVarString > m_reflectedVar;
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
    QScopedPointer<CReflectedVarBool > m_reflectedVar;
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
    QScopedPointer<CReflectedVarEnum<AZStd::string>  > m_reflectedVar;

    IVariable* m_pVariable;
    IVarEnumListPtr m_enumList;
    bool m_updatingEnums;
};

class EDITOR_CORE_API ReflectedVarColor3Adapter : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }

private:
    QScopedPointer<CReflectedVarVector3> m_reflectedVar;
};

class EDITOR_CORE_API ReflectedVarColor4Adapter : public ReflectedVarAdapter
{
public:
    void SetVariable(IVariable* pVariable) override;
    void SyncReflectedVarToIVar(IVariable* pVariable) override;
    void SyncIVarToReflectedVar(IVariable* pVariable) override;
    CReflectedVar* GetReflectedVar() override { return m_reflectedVar.data(); }

private:
    QScopedPointer<CReflectedVarVector4> m_reflectedVar;
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
    QScopedPointer<CReflectedVarVector2 > m_reflectedVar;
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
    QScopedPointer<CReflectedVarVector3 > m_reflectedVar;
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
    QScopedPointer<CReflectedVarVector4 > m_reflectedVar;
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
    QScopedPointer<CReflectedVarResource> m_reflectedVar;
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
    QScopedPointer<CReflectedVarUser> m_reflectedVar;
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
    QScopedPointer<CReflectedVarSpline > m_reflectedVar;
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
    QScopedPointer<CReflectedVarGenericProperty > m_reflectedVar;
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
    QScopedPointer<CReflectedVarMotion > m_reflectedVar;
};


#endif // CRYINCLUDE_EDITOR_UTILS_REFLECTEDVARWRAPPER_H
