/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_UTILS_REFLECTEDPROPERTYITEM_H
#define CRYINCLUDE_EDITOR_UTILS_REFLECTEDPROPERTYITEM_H
#pragma once

#include "Util/Variable.h"
#include <Util/VariablePropertyType.h>

namespace AzToolsFramework {
    class ReflectedPropertyEditor;
}

class CReflectedVar;
class CPropertyContainer;
class ReflectedPropertyControl;
class ReflectedVarAdapter;
class ReflectedVarContainerAdapter;

// Class representing a property inside a ReflectedPropertyCtrl.
// It contains the IVariable and corresponding CReflectedVar for that property
// and any children properties if the IVariable is a container.

// This class is loosely based on the MFC-based CPropertyItem to make porting easier.
// The CPropertyItem created editor widgets for each property type, but this class
// just holds a CReflectedVar and updates it's values. The editing is done by the
// reflection system and registered property handlers for each CReflectedVar.
class EDITOR_CORE_API ReflectedPropertyItem
    : public CRefCountBase
{
public:
    ReflectedPropertyItem(ReflectedPropertyControl* control, ReflectedPropertyItem* parent);
    ~ReflectedPropertyItem();

    void SetVariable(IVariable* var);
    IVariable* GetVariable() const { return m_pVariable; }

    void ReplaceVarBlock(CVarBlock* varBlock);

    CReflectedVar* GetReflectedVar() const;

    ReflectedPropertyItem* findItem(CReflectedVar* var);
    ReflectedPropertyItem* findItem(IVariable* var);
    ReflectedPropertyItem* findItem(const QString &name);
    ReflectedPropertyItem* FindItemByFullName(const QString& fullName);

    //update the internal IVariable as result of ReflectedVar changing
    void OnReflectedVarChanged();

    //update the ReflectedVar to current value of IVar
    void SyncReflectedVarToIVar();

    //! Return true if this property item is modified.
    bool IsModified() const { return m_modified; }

    void ReloadValues();

    ReflectedVarContainerAdapter* GetContainer() { return m_reflectedVarContainerAdapter; }
    ReflectedPropertyItem* GetParent() { return m_parent; }

    /** Get script default value of property item.
    */
    virtual bool HasScriptDefault() const { return m_strScriptDefault != m_strNoScriptDefault; };

    /** Get script default value of property item.
    */
    virtual QString GetScriptDefault() const { return m_strScriptDefault; };

    /** Set script default value of property item.
    */
    virtual void SetScriptDefault(const QString& sScriptDefault) { m_strScriptDefault = sScriptDefault; };

    /** Set script default value of property item.
    */
    virtual void ClearScriptDefault() { m_strScriptDefault = m_strNoScriptDefault; };


    /** Changes value of item.
    */
    virtual void SetValue(const QString& sValue, bool bRecordUndo = true, bool bForceModified = false);

    //hack for calling ReflectedPropertyControl::OnItemChange from a wrapper class
    //this is used because changes to Splines should not actually change anything in the IVariable,
    //but we need OnItemChanged as if the IVariable did change.
    void SendOnItemChange();

    void ExpandAllChildren(bool recursive);
    void Expand(bool expand);

    QString GetPropertyName() const;

    void AddChild(ReflectedPropertyItem* item);
    void RemoveAllChildren();
    void RemoveChild(ReflectedPropertyItem* item);

    // default number of increments to cover the range of a property
    static const float s_DefaultNumStepIncrements;

    // for a consistent Feel, compute the step size for a numerical slider for the specified min/max, rounded to precision
    inline static float ComputeSliderStep(float sliderMin, float sliderMax, const float precision = .01f)
    {
        float step;
        step = int_round(((sliderMax - sliderMin) / ReflectedPropertyItem::s_DefaultNumStepIncrements) / precision) * precision;
        // prevent rounding down to zero
        return (step > precision) ? step : precision;
    }

protected:
    friend class ReflectedPropertyControl;

    //! Release used variable.
    void ReleaseVariable();
    //! Callback called when variable change.
    void OnVariableChange(IVariable* var);
    void OnVariableEnumChange(IVariable* var);

public:
    //! Get number of child nodes.
    int GetChildCount() const { return static_cast<int>(m_childs.size()); };
    //! Get Child by id.
    ReflectedPropertyItem* GetChild(int index) const { return m_childs[index]; }
    PropertyType GetType() const { return m_type; }

    /** Get name of property item.
    */
    virtual QString GetName() const;

    QString GetFullName() const;

protected:
    PropertyType m_type;

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //The variable being edited.
    _smart_ptr<IVariable> m_pVariable;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    //holds the CReflectedVar and syncs its value with IVariable when either changes
    ReflectedVarAdapter* m_reflectedVarAdapter;
    ReflectedVarContainerAdapter* m_reflectedVarContainerAdapter;

    ReflectedPropertyItem* m_parent;
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    std::vector<_smart_ptr<ReflectedPropertyItem> > m_childs;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    ReflectedPropertyControl* m_propertyCtrl;

    unsigned int m_modified : 1;

    bool m_syncingIVar;

    QString m_strNoScriptDefault;
    QString m_strScriptDefault;

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    IVariable::OnSetCallback m_onSetCallback;
    IVariable::OnSetEnumCallback m_onSetEnumCallback;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

typedef _smart_ptr<ReflectedPropertyItem> ReflectedPropertyItemPtr;


#endif // CRYINCLUDE_EDITOR_UTILS_REFLECTEDPROPERTYITEM_H
