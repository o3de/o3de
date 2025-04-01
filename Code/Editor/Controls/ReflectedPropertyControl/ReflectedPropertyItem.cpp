/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ReflectedPropertyItem.h"

// AzToolsFramework
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>

// Editor
#include "ReflectedVarWrapper.h"
#include "ReflectedPropertyCtrl.h"
#include "Undo/UndoVariableChange.h"

#include <AzCore/Serialization/Locale.h>

// default number of increments to cover the range of a property - determined experimentally by feel
const float ReflectedPropertyItem::s_DefaultNumStepIncrements = 500.0f;


//A ReflectedVarAdapter for holding IVariableContainers

//The extra ReflectedVarAdapter is the extra case of a container (has children) but also has a value itself.
//An example is an IVariable array whose type is forced to IVariable::DT_TEXTURE. The base Ivariable has a texture,
//but it also has children that are parameters of the texture. The ReflectedPropertyEditor does not support this case
//so we work around by adding the base property to the list of children and showing the value of the base property
//in the container value space instead of "X Elements"

static ColorF StringToColor(const QString &value)
{
    // note that converting back and forth between these values is happening in 2 cases
    // one is when its being read from the UI and the other is when it is being read from XML
    // XML is always in "C" locale, and the GUI is ultimately using @ref type_convertor in Variable.h
    // which itself uses QString::number, which is always in the "C" locale (invariant culture).
    // for example, color is converted using the following line
    // void operator()(const Vec4& value, QString& to) const { to = QString::fromLatin1("%1,%2,%3,%4").arg(value.x).arg(value.y).arg(value.z).arg(value.w); }
    // qstring::arg uses the "C" Locale unless explicitly using the notation %L1, %L2, etc, so the input will be invariant.

    AZ::Locale::ScopedSerializationLocale localeScope;

    ColorF color;
    float r, g, b, a;
    int res = azsscanf(value.toUtf8().data(), "%f,%f,%f,%f", &r, &g, &b, &a);
    if (res == 4)
    {
        color.Set(r, g, b, a);
    }
    else if (res == 3)
    {
        color.Set(r, g, b);
    }
    else
    {
        unsigned abgr;
        azsscanf(value.toUtf8().data(), "%u", &abgr);
        color = ColorF(abgr);
    }
    return color;
}

class ReflectedVarContainerAdapter : public ReflectedVarAdapter
{
public:
    ReflectedVarContainerAdapter(ReflectedPropertyItem *item, ReflectedPropertyControl *control, ReflectedVarAdapter *variableAdapter = nullptr)
        : m_extraVariableAdapter(variableAdapter)
        , m_item(item)
        , m_propertyCtrl(control)
        , m_containerVar(new CPropertyContainer(AZStd::string()))
    {
        m_containerVar->SetAutoExpand(false);
    }

    void SetVariable(IVariable *pVariable) override
    {
        if (m_extraVariableAdapter)
            m_extraVariableAdapter->SetVariable(pVariable);

        //Check whether the parent container has autoExpand flag set, and if so, the autoexpand flag for this item
        //We need to do this because the default IVariable flags has the item expanded, so most items are expanded,
        //but the ReflectedPropertyEditor expands all ancestors if any item is expanded.
        //This is not what we want -- the old property editor did not expand ancestors. In case of Material editor,
        //this expansion can be really expensive!
        const bool parentIsAutoExpand = m_item->GetParent() == nullptr || m_item->GetParent()->GetContainer() == nullptr || m_item->GetParent()->GetContainer()->m_containerVar->AutoExpand();
        const bool bDefaultExpand = (pVariable->GetFlags() & IVariable::UI_COLLAPSED) == 0 || (pVariable->GetFlags() & IVariable::UI_AUTO_EXPAND);
        m_containerVar->SetAutoExpand(parentIsAutoExpand && bDefaultExpand);

        UpdateCommon(pVariable, pVariable);
    }

    //helps implement ReflectedPropertyControl::ReplaceVarBlock
    void ReplaceVarBlock(CVarBlock *varBlock) override
    {
        m_containerVar->Clear();
        UpdateCommon(m_item->GetVariable(), varBlock);
    }

    void SyncReflectedVarToIVar(IVariable *pVariable) override
    {
        if (m_extraVariableAdapter)
        {
            m_extraVariableAdapter->SyncReflectedVarToIVar(pVariable);
            //update text on parent container. Do not have control update attributes since this will happen anyway as part of updating ReflectedVar
            updateContainerText(pVariable, false);
        }
    };

    void SyncIVarToReflectedVar(IVariable *pVariable) override
    {
        if (m_extraVariableAdapter)
        {
            m_extraVariableAdapter->SyncIVarToReflectedVar(pVariable);
            //update text on parent container. Force control to update attributes since this doesn't normally happen when updating an IVar from ReflectedVar
            updateContainerText(pVariable, true);
        }
    };

    CReflectedVar *GetReflectedVar() override { return m_containerVar.data(); }

    bool Contains(CReflectedVar *var) override { return var == m_containerVar.data() || (m_extraVariableAdapter && m_extraVariableAdapter->GetReflectedVar() == var); }

private:

    void UpdateCommon(IVariable *nameVariable, IVariableContainer *childContainer)
    {
        m_containerVar->m_varName = nameVariable->GetHumanName().toUtf8().data();
        m_containerVar->m_description = nameVariable->GetDescription().toUtf8().data();
        if (m_extraVariableAdapter)
        {
            m_containerVar->AddProperty(m_extraVariableAdapter->GetReflectedVar());
        }
        //Handle adding empty varblock
        if (!childContainer)
            return;

        for (int i = 0; i < childContainer->GetNumVariables(); i++)
        {
            AddChild(childContainer->GetVariable(i));
        }
    }

    void AddChild(IVariable *var)
    {
        if (var->GetFlags() & IVariable::UI_INVISIBLE)
            return;
        ReflectedPropertyItemPtr item = new ReflectedPropertyItem(m_propertyCtrl, m_item);
        item->SetVariable(var);
        m_containerVar->AddProperty(item->GetReflectedVar());
    }

    void updateContainerText(IVariable *pVariable, bool updateAttributes)
    {
        //set text of the container to the value of the main variable.  If it's empty, use space, otherwise ReflectedPropertyEditor doesn't update it!
        m_containerVar->SetValueText(pVariable->GetDisplayValue().isEmpty() ? AZStd::string(" ") : AZStd::string(pVariable->GetDisplayValue().toUtf8().data()));
        if (updateAttributes)
            m_propertyCtrl->InvalidateCtrl();
    }

private:
    //optional adapter for case where this item contains a variable in addition to a container of variables.
    ReflectedVarAdapter *m_extraVariableAdapter;
    QScopedPointer<CPropertyContainer> m_containerVar;
    ReflectedPropertyItem *m_item;
    ReflectedPropertyControl *m_propertyCtrl;
};

ReflectedPropertyItem::ReflectedPropertyItem(ReflectedPropertyControl *control, ReflectedPropertyItem *parent)
    : m_pVariable(nullptr)
    , m_reflectedVarAdapter(nullptr)
    , m_reflectedVarContainerAdapter(nullptr)
    , m_parent(parent)
    , m_propertyCtrl(control)
    , m_syncingIVar(false)
    , m_strNoScriptDefault("<<undefined>>")
    , m_strScriptDefault(m_strNoScriptDefault)
{
    m_type = ePropertyInvalid;
    m_modified = false;
    if (parent)
        parent->AddChild(this);

    m_onSetCallback = [this](IVariable* var) { OnVariableChange(var); };
    m_onSetEnumCallback = [this](IVariable* var) { OnVariableEnumChange(var); };
}

ReflectedPropertyItem::~ReflectedPropertyItem()
{
    // just to make sure we dont double (or infinitely recurse...) delete
    AddRef();

    if (m_pVariable)
        ReleaseVariable();

    RemoveAllChildren();
}

void ReflectedPropertyItem::SetVariable(IVariable *var)
{
    if (var == m_pVariable)
    {
        // Early exit optimization if setting the same var as the current var.
        // A common use case, in Track View for example, is to re-use the save var for a property when switching to a new
        // instance of the same variable. The visible display of the value is often handled by invalidating the property,
        // but the non-visible attributes, i.e. the range values, are usually set using this method. Thus we reset the ranges
        // explicitly here when the Ivariable var is the same

        if (m_reflectedVarAdapter)
            m_reflectedVarAdapter->UpdateRangeLimits(var);
        return;
    }
    _smart_ptr<IVariable> pInputVar = var;
    // Release previous variable.
    if (m_pVariable)
        ReleaseVariable();

    m_pVariable = pInputVar;
    assert(m_pVariable != nullptr);

    m_pVariable->AddOnSetCallback(&m_onSetCallback);
    m_pVariable->AddOnSetEnumCallback(&m_onSetEnumCallback);

    // Fetch base parameter description
    Prop::Description desc(m_pVariable);
    m_type = desc.m_type;

    switch (m_type)
    {
    case ePropertyVector2:
        m_reflectedVarAdapter = new ReflectedVarVector2Adapter;
        break;
    case ePropertyVector:
        m_reflectedVarAdapter = new ReflectedVarVector3Adapter;
        break;
    case ePropertyVector4:
        m_reflectedVarAdapter = new ReflectedVarVector4Adapter;
        break;
    case ePropertyFloat:
    case ePropertyAngle:
        m_reflectedVarAdapter = new ReflectedVarFloatAdapter;
        break;
    case ePropertyInt:
        m_reflectedVarAdapter = new ReflectedVarIntAdapter;
        break;
    case ePropertyBool:
        m_reflectedVarAdapter = new ReflectedVarBoolAdapter;
        break;
    case ePropertyString:
        m_reflectedVarAdapter = new ReflectedVarStringAdapter;
        break;
    case ePropertySelection:
        m_reflectedVarAdapter = new ReflectedVarEnumAdapter;
        break;
    case ePropertyUser:
        m_reflectedVarAdapter = new ReflectedVarUserAdapter;
        break;
    case ePropertyEquip:
    case ePropertyGameToken:
    case ePropertyMissionObj:
    case ePropertySequence:
    case ePropertySequenceId:
    case ePropertyLocalString:
    case ePropertyLightAnimation:
    case ePropertyParticleName:
        m_reflectedVarAdapter = new ReflectedVarGenericPropertyAdapter(desc.m_type);
        break;
    case ePropertyTexture:
    case ePropertyAudioTrigger:
    case ePropertyAudioSwitch:
    case ePropertyAudioSwitchState:
    case ePropertyAudioRTPC:
    case ePropertyAudioEnvironment:
    case ePropertyAudioPreloadRequest:
        m_reflectedVarAdapter = new ReflectedVarResourceAdapter;
        break;
    case ePropertyFloatCurve:
    case ePropertyColorCurve:
        m_reflectedVarAdapter = new ReflectedVarSplineAdapter(this, m_type);
        break;
    case ePropertyMotion:
        m_reflectedVarAdapter = new ReflectedVarMotionAdapter;
        break;
    default:
        break;
    }

    const bool hasChildren = (m_pVariable->GetNumVariables() > 0 || desc.m_type == ePropertyTable || m_pVariable->GetType() == IVariable::ARRAY);
    //const bool isNotContainerType = (m_pVariable->GetType() != IVariable::ARRAY && desc.m_type != ePropertyTable  && desc.m_type != ePropertyInvalid);
    if (hasChildren )
    {
        m_reflectedVarContainerAdapter = new ReflectedVarContainerAdapter(this, m_propertyCtrl, m_reflectedVarAdapter);
        m_reflectedVarAdapter = m_reflectedVarContainerAdapter;
    }

    if (m_reflectedVarAdapter)
    {
        m_reflectedVarAdapter->SetVariable(m_pVariable);
        m_reflectedVarAdapter->SyncReflectedVarToIVar(m_pVariable);
    }

    m_modified = false;
}

void ReflectedPropertyItem::ReplaceVarBlock(CVarBlock *varBlock)
{
    RemoveAllChildren();
    if (m_reflectedVarAdapter)
        m_reflectedVarAdapter->ReplaceVarBlock(varBlock);
}

void ReflectedPropertyItem::AddChild(ReflectedPropertyItem *item)
{
    assert(item);
    m_childs.push_back(item);
}

void ReflectedPropertyItem::RemoveAllChildren()
{
    for (int i = 0; i < m_childs.size(); i++)
    {
        m_childs[i]->m_parent = nullptr;
    }

    m_childs.clear();
}

void ReflectedPropertyItem::RemoveChild(ReflectedPropertyItem* item)
{
    for (int i = 0; i < m_childs.size(); i++)
    {
        if (m_childs[i] == item)
        {
            item->m_parent = nullptr;
            m_childs.erase(m_childs.begin() + i);
            return;
        }
    }
}

CReflectedVar * ReflectedPropertyItem::GetReflectedVar() const
{
    return m_reflectedVarAdapter ? m_reflectedVarAdapter->GetReflectedVar() : nullptr;
}

ReflectedPropertyItem * ReflectedPropertyItem::findItem(CReflectedVar *var)
{
    if (m_reflectedVarAdapter && m_reflectedVarAdapter->Contains(var) )
        return this;
    for (auto child : m_childs)
    {
        ReflectedPropertyItem *result = child->findItem(var);
        if (result)
            return result;
    }
    return nullptr;
}


ReflectedPropertyItem * ReflectedPropertyItem::findItem(IVariable *var)
{
    if (m_pVariable == var)
        return this;
    for (auto child : m_childs)
    {
        ReflectedPropertyItem *result = child->findItem(var);
        if (result)
            return result;
    }
    return nullptr;
}

ReflectedPropertyItem* ReflectedPropertyItem::findItem(const QString &name)
{
    if (m_pVariable && m_pVariable->GetHumanName() == name)
        return this;
    for (auto child : m_childs)
    {
        ReflectedPropertyItem *result = child->findItem(name);
        if (result)
            return result;
    }
    return nullptr;

}

ReflectedPropertyItem * ReflectedPropertyItem::FindItemByFullName(const QString& fullName)
{
    if (GetFullName() == fullName)
    {
        return this;
    }
    for (int i = 0; i < m_childs.size(); ++i)
    {
        auto pFound = m_childs[i]->FindItemByFullName(fullName);
        if (pFound)
        {
            return pFound;
        }
    }
    return nullptr;
}

QString ReflectedPropertyItem::GetName() const
{
    return m_pVariable ? m_pVariable->GetHumanName() : QString();
}

QString ReflectedPropertyItem::GetFullName() const
{
    if (m_parent && m_pVariable)
    {
        return m_parent->GetFullName() + "::" + m_pVariable->GetName();
    }
    else if (m_pVariable)
    {
        return m_pVariable->GetName();
    }
    else
    {
        return {};
    }
}

void ReflectedPropertyItem::OnReflectedVarChanged()
{
    m_syncingIVar = true;
    if (m_reflectedVarAdapter)
    {
        std::unique_ptr<CUndo> undo;
        if (!CUndo::IsRecording())
        {
            if (!m_propertyCtrl->CallUndoFunc(this))
                undo.reset(new CUndo((m_pVariable->GetHumanName() + " Modified").toUtf8().data()));
        }

        m_reflectedVarAdapter->SyncIVarToReflectedVar(m_pVariable);

        if (m_propertyCtrl->IsStoreUndoByItems() && CUndo::IsRecording())
            CUndo::Record(new CUndoVariableChange(m_pVariable, "PropertyChange"));

        m_modified = true;
    }
    m_syncingIVar = false;
}

void ReflectedPropertyItem::SyncReflectedVarToIVar()
{
    if (m_reflectedVarAdapter)
    {
        m_reflectedVarAdapter->SyncReflectedVarToIVar(m_pVariable);
    }
}

void ReflectedPropertyItem::ReleaseVariable()
{
    if (m_pVariable)
    {
        // Unwire all from variable.
        m_pVariable->RemoveOnSetCallback(&m_onSetCallback);
        m_pVariable->RemoveOnSetEnumCallback(&m_onSetEnumCallback);
    }
    m_pVariable = nullptr;
    delete m_reflectedVarAdapter;
    m_reflectedVarAdapter = nullptr;
}

void ReflectedPropertyItem::OnVariableChange(IVariable* pVar)
{
    assert(pVar != 0 && pVar == m_pVariable);

    if (m_syncingIVar)
        return;

    // When variable changes, invalidate UI.
    m_modified = true;

    if (m_reflectedVarAdapter)
    {
        m_reflectedVarAdapter->OnVariableChange(pVar);
    }
    SyncReflectedVarToIVar();

    m_propertyCtrl->InvalidateCtrl();
}

void ReflectedPropertyItem::OnVariableEnumChange([[maybe_unused]] IVariable* pVar)
{

    if (m_reflectedVarAdapter && m_reflectedVarAdapter->UpdateReflectedVarEnums())
    {
        m_propertyCtrl->InvalidateCtrl(true);
    }
}

void ReflectedPropertyItem::ReloadValues()
{
    m_modified = false;

    if (m_pVariable)
        SetVariable(m_pVariable);

    for (int i = 0; i < GetChildCount(); i++)
    {
        GetChild(i)->ReloadValues();
    }
    SyncReflectedVarToIVar();
}


/** Changes value of item.
*/
void ReflectedPropertyItem::SetValue(const QString& sValue, bool bRecordUndo, bool bForceModified)
{
    if (!m_pVariable)
    {
        return;
    }

    _smart_ptr<ReflectedPropertyItem> holder = this; // Make sure we are not released during this function.

    QString value = sValue;

    switch (m_type)
    {
    case ePropertyBool:
        if (QString::compare(value, "true", Qt::CaseInsensitive) == 0 || value.toInt() != 0)
        {
            value = "1";
        }
        else
        {
            value = "0";
        }
        break;

    case ePropertyVector2:
        if (!value.contains(','))
        {
            value = value + ", " + value;
        }
        break;

    case ePropertyVector4:
        if (!value.contains(','))
        {
            value = value + ", " + value + ", " + value + ", " + value;
        }
        break;

    case ePropertyVector:
        if (!value.contains(','))
        {
            value = value + ", " + value + ", " + value;
        }
        break;

    case ePropertyTexture:
        value.replace('\\', '/');
        break;
    }

    // correct the length of value
    switch (m_type)
    {
    case ePropertyTexture:
        if (value.length() >= MAX_PATH)
        {
            value = value.left(MAX_PATH);
        }
        break;
    }


    bool bModified = bForceModified || m_pVariable->GetDisplayValue() != value;
    bool bStoreUndo = (m_pVariable->GetDisplayValue() != value || bForceModified) && bRecordUndo;

    std::unique_ptr<CUndo> undo;
    if (bStoreUndo && !CUndo::IsRecording())
    {
        if (!m_propertyCtrl->CallUndoFunc(this))
        {
            undo.reset(new CUndo((GetName() + " Modified").toUtf8().data()));
        }
    }

    if (m_pVariable)
    {
        if (bModified)
        {
            if (m_propertyCtrl->IsStoreUndoByItems() && bStoreUndo && CUndo::IsRecording())
            {
                CUndo::Record(new CUndoVariableChange(m_pVariable, "PropertyChange"));
            }

            if (bForceModified)
            {
                m_pVariable->SetForceModified(true);
            }

            switch (m_type)
            {
            case ePropertyColor:
            {
                ColorF color = StringToColor(value);
                if (m_pVariable->GetType() == IVariable::VECTOR)
                {
                    m_pVariable->Set(color.toVec3());
                }
                else
                {
                    m_pVariable->Set(static_cast<int>(color.pack_abgr8888()));
                }
                break;
            }
            case ePropertyInvalid:
                break;
            default:
                m_pVariable->SetDisplayValue(value);
                break;
            }
        }
    }
    else
    {
        if (bModified)
        {
            m_modified = true;
            // If Value changed mark document modified.
            // Notify parent that this Item have been modified.
            m_propertyCtrl->OnItemChange(this);
        }
    }
}

void ReflectedPropertyItem::SendOnItemChange()
{
    m_propertyCtrl->OnItemChange(this);
}

void ReflectedPropertyItem::ExpandAllChildren(bool recursive)
{
    Expand(true);
    for (auto child : m_childs)
    {
        if (recursive)
        {
            child->ExpandAllChildren(recursive);
        }
        else
        {
            child->Expand(true);
        }
    }
}

void ReflectedPropertyItem::Expand(bool expand)
{
    AzToolsFramework::PropertyRowWidget *widget = m_propertyCtrl->FindPropertyRowWidget(this);
    if (widget)
    {
        widget->SetExpanded(expand);
    }
}

QString ReflectedPropertyItem::GetPropertyName() const
{
    return m_pVariable ? m_pVariable->GetHumanName() : QString();
}

