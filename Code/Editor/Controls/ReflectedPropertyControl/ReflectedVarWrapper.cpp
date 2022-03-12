/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ReflectedVarWrapper.h"

// AzCore
#include <AzCore/Asset/AssetManagerBus.h>

// Editor
#include "ReflectedPropertyCtrl.h"
#include "UIEnumsDatabase.h"

namespace {

    //setting the IVariable to itself in property items was trigger to update limits for that variable. 
    //limits were obtained using IVariable::GetLimits instead of from the Prop::Description
    template <class T, class R>
    void setRangeParams(CReflectedVarRanged<T, R> *reflectedVar, IVariable *pVariable, bool updatingExistingVariable = false)
    {
        float min, max, step;
        bool  hardMin, hardMax;
        if (updatingExistingVariable)
        {
            pVariable->GetLimits(min, max, step, hardMin, hardMax);
        }
        else
        {
            Prop::Description desc(pVariable);
            min = desc.m_rangeMin;
            max = desc.m_rangeMax;
            step = desc.m_step;
            hardMin = desc.m_bHardMin;
            hardMax = desc.m_bHardMax;
        }
        reflectedVar->m_softMinVal = static_cast<R>(min);
        reflectedVar->m_softMaxVal = static_cast<R>(max);

        if (hardMin)
        {
            reflectedVar->m_minVal = static_cast<R>(min);
        }
        else
        {
            reflectedVar->m_minVal = std::numeric_limits<R>::lowest();
        }
        if (hardMax)
        {
            reflectedVar->m_maxVal = static_cast<R>(max);
        }
        else
        {
            // There is an issue with assigning std::numeric_limits<int>::max() to a float
            // A float can't actually represent the value of 2147483647 and clang
            // compilers actually warn on this fact.
            // A static_cast is used here to indicate explicit acceptance of the value change here
            /* The clang compiler warning is below
              ../Code/Editor/Controls/ReflectedPropertyControl/ReflectedVarWrapper.cpp:59:38: error: implicit conversion from 'int' to 'float' changes value from 2147483647 to 2147483648 [-Werror,-Wimplicit-int-float-conversion]
              reflectedVar->m_maxVal = std::numeric_limits<int>::max();
            */
            reflectedVar->m_maxVal = static_cast<R>(std::numeric_limits<int>::max());
        }
        reflectedVar->m_stepSize = static_cast<R>(step);
    }
}

void ReflectedVarIntAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarInt(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
    UpdateRangeLimits(pVariable);
    Prop::Description desc(pVariable);
    m_valueMultiplier = desc.m_valueMultiplier;
}

void ReflectedVarIntAdapter::UpdateRangeLimits(IVariable *pVariable)
{
    setRangeParams<int>(m_reflectedVar.data(), pVariable);
}

void ReflectedVarIntAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    float value;
    if (pVariable->GetType() == IVariable::FLOAT)
    {
        pVariable->Get(value);
    }
    else
    {
        int intValue;
        pVariable->Get(intValue);
        value = static_cast<float>(intValue);
    }
    m_reflectedVar->m_value = static_cast<int>(std::round(value * m_valueMultiplier));
}

void ReflectedVarIntAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    //don't round here.  Often the IVariable is actually a float under-the hood
    //for example: DT_PERCENT is stored in float (0 to 1) but has ePropertyType::Integer because editor should be an integer editor ranging from 0 to 100.
    pVariable->Set(m_reflectedVar->m_value / m_valueMultiplier);
}



void ReflectedVarFloatAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarFloat(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
    UpdateRangeLimits(pVariable);
    Prop::Description desc(pVariable);
    m_valueMultiplier = desc.m_valueMultiplier;
}

void ReflectedVarFloatAdapter::UpdateRangeLimits(IVariable *pVariable)
{
    setRangeParams<float>(m_reflectedVar.data(), pVariable);
}

void ReflectedVarFloatAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    float value;
    pVariable->Get(value);
    m_reflectedVar->m_value = value * m_valueMultiplier;
}

void ReflectedVarFloatAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(m_reflectedVar->m_value/m_valueMultiplier);
}



void ReflectedVarStringAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarString(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
}

void ReflectedVarStringAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    QString value;
    pVariable->Get(value);
    m_reflectedVar->m_value = value.toUtf8().data();
}

void ReflectedVarStringAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(m_reflectedVar->m_value.c_str());
}




void ReflectedVarBoolAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarBool(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
}

void ReflectedVarBoolAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    bool value;
    pVariable->Get(value);
    m_reflectedVar->m_value = value;
}

void ReflectedVarBoolAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(m_reflectedVar->m_value);
}




ReflectedVarEnumAdapter::ReflectedVarEnumAdapter()
    : m_updatingEnums(false)
    , m_pVariable(nullptr)
{}

void ReflectedVarEnumAdapter::SetVariable(IVariable *pVariable)
{
    m_pVariable = pVariable;
    Prop::Description desc(pVariable);
    m_enumList = desc.m_enumList;
    m_reflectedVar.reset(new CReflectedVarEnum<AZStd::string>(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
    UpdateReflectedVarEnums();
}

bool ReflectedVarEnumAdapter::UpdateReflectedVarEnums()
{
    if (!m_pVariable || m_updatingEnums)
    {
        return false;
    }

    m_updatingEnums = true;
    //Allow derived classes to populate the IVariable's enumList
    updateIVariableEnumList(m_pVariable);
    m_enumList = m_pVariable->GetEnumList();
    m_updatingEnums = false;

    bool changed = false;
    //Copy the updated enums to the ReflecteVar
    if (m_enumList)
    {
        const AZStd::vector<AZStd::string> oldEnums = m_reflectedVar->GetEnums();

        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> enums;
        for (uint i = 0; !m_enumList->GetItemName(i).isNull(); i++)
        {
            QString sEnumName = m_enumList->GetItemName(i);
            enums.push_back(AZStd::pair<AZStd::string, AZStd::string>(sEnumName.toUtf8().data(), sEnumName.toUtf8().data()));

        }
        m_reflectedVar->setEnums(enums);

        changed = m_reflectedVar->GetEnums() != oldEnums;
        if (changed)
        {
            // set the current enum value from the IVariable
            SyncReflectedVarToIVar(m_pVariable);
        }
    }
    return changed;
}

void ReflectedVarEnumAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    const AZStd::string value = pVariable->GetDisplayValue().toUtf8().data();
    m_reflectedVar->setEnumByName(value);
}

void ReflectedVarEnumAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    QString iVarVal = m_reflectedVar->m_selectedEnumName.c_str();
    pVariable->SetDisplayValue(iVarVal);
}

void ReflectedVarEnumAdapter::OnVariableChange([[maybe_unused]] IVariable* pVariable)
{
    //setting the enums on the pVariable will cause the variable to change getting us back here
    //The original property editor did need to update things immediately because it did so when creating the in-place editing control
    if (!m_updatingEnums)
    {
        UpdateReflectedVarEnums();
    }
}

void ReflectedVarDBEnumAdapter::SetVariable(IVariable *pVariable)
{
    Prop::Description desc(pVariable);
    m_pEnumDBItem = desc.m_pEnumDBItem;
    m_reflectedVar.reset(new CReflectedVarEnum<AZStd::string>(pVariable->GetHumanName().toUtf8().data()));
    if (m_pEnumDBItem)
    {
        for (int i = 0; i < m_pEnumDBItem->strings.size(); i++)
        {
            QString name = m_pEnumDBItem->strings[i];
            m_reflectedVar->addEnum( m_pEnumDBItem->NameToValue(name).toUtf8().data(), name.toUtf8().data() );
        }
    }
}

void ReflectedVarDBEnumAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    const AZStd::string valueStr = pVariable->GetDisplayValue().toUtf8().data();
    const AZStd::string value = m_pEnumDBItem ? AZStd::string(m_pEnumDBItem->ValueToName(valueStr.c_str()).toUtf8().data()) : valueStr;
    m_reflectedVar->setEnumByName(value);

}

void ReflectedVarDBEnumAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    QString iVarVal = m_reflectedVar->m_selectedEnumName.c_str();
    if (m_pEnumDBItem)
    {
        iVarVal = m_pEnumDBItem->NameToValue(iVarVal);
    }
    pVariable->SetDisplayValue(iVarVal);
}



void ReflectedVarVector2Adapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarVector2(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
    UpdateRangeLimits(pVariable);
}

void ReflectedVarVector2Adapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    Vec2 vec;
    pVariable->Get(vec);
    m_reflectedVar->m_value = AZ::Vector2(vec.x, vec.y);
}

void ReflectedVarVector2Adapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(Vec2(m_reflectedVar->m_value.GetX(), m_reflectedVar->m_value.GetY()));
}


void ReflectedVarVector3Adapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarVector3(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
    UpdateRangeLimits(pVariable);
}

void ReflectedVarVector3Adapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    Vec3 vec;
    pVariable->Get(vec);
    m_reflectedVar->m_value = AZ::Vector3(vec.x, vec.y, vec.z);
}

void ReflectedVarVector3Adapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(Vec3(m_reflectedVar->m_value.GetX(), m_reflectedVar->m_value.GetY(), m_reflectedVar->m_value.GetZ()));
}


void ReflectedVarVector4Adapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarVector4(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
    UpdateRangeLimits(pVariable);
}

void ReflectedVarVector4Adapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    Vec4 vec;
    pVariable->Get(vec);
    m_reflectedVar->m_value = AZ::Vector4(vec.x, vec.y, vec.z, vec.w);
}

void ReflectedVarVector4Adapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(Vec4(m_reflectedVar->m_value.GetX(), m_reflectedVar->m_value.GetY(), m_reflectedVar->m_value.GetZ(), m_reflectedVar->m_value.GetW()));
}


void ReflectedVarColorAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarColor(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
}

void ReflectedVarColorAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    if (pVariable->GetType() == IVariable::VECTOR)
    {
        Vec3 v(0, 0, 0);
        pVariable->Get(v);
        const QColor col = ColorLinearToGamma(ColorF(v.x, v.y, v.z));
        m_reflectedVar->m_color.Set(static_cast<float>(col.redF()), static_cast<float>(col.greenF()), static_cast<float>(col.blueF()));
    }
    else
    {
        int col(0);
        pVariable->Get(col);
        const QColor qcolor = ColorToQColor((uint32)col);
        m_reflectedVar->m_color.Set(static_cast<float>(qcolor.redF()), static_cast<float>(qcolor.greenF()), static_cast<float>(qcolor.blueF()));
    }
}

void ReflectedVarColorAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    if (pVariable->GetType() == IVariable::VECTOR)
    {
        ColorF colLin = ColorGammaToLinear(QColor::fromRgbF(m_reflectedVar->m_color.GetX(), m_reflectedVar->m_color.GetY(), m_reflectedVar->m_color.GetZ()));
        pVariable->Set(Vec3(colLin.r, colLin.g, colLin.b));
    }
    else
    {
        int ir = static_cast<int>(m_reflectedVar->m_color.GetX() * 255.0f);
        int ig = static_cast<int>(m_reflectedVar->m_color.GetY() * 255.0f);
        int ib = static_cast<int>(m_reflectedVar->m_color.GetZ() * 255.0f);

        pVariable->Set(static_cast<int>(RGB(ir, ig, ib)));
    }
}



void ReflectedVarResourceAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarResource(pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
}

void ReflectedVarResourceAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    QString path;
    pVariable->Get(path);
    m_reflectedVar->m_path = path.toUtf8().data();
    Prop::Description desc(pVariable);
    m_reflectedVar->m_propertyType = desc.m_type;

}

void ReflectedVarResourceAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    const bool bForceModified = false;
    pVariable->SetForceModified(bForceModified);
    pVariable->SetDisplayValue(m_reflectedVar->m_path.c_str());

    //shouldn't be able to change the type, so ignore m_reflecatedVar->m_properyType
}


ReflectedVarGenericPropertyAdapter::ReflectedVarGenericPropertyAdapter(PropertyType propertyType)
    :m_propertyType(propertyType)
{}

void ReflectedVarGenericPropertyAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarGenericProperty(m_propertyType, pVariable->GetHumanName().toUtf8().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toUtf8().data();
}

void ReflectedVarGenericPropertyAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    QString value;
    pVariable->Get(value);

    m_reflectedVar->m_value = value.toUtf8().data();
}

void ReflectedVarGenericPropertyAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(m_reflectedVar->m_value.c_str());
}

void ReflectedVarUserAdapter::SetVariable(IVariable *pVariable)
{
    m_reflectedVar.reset(new CReflectedVarUser( pVariable->GetHumanName().toUtf8().data()));
}

void ReflectedVarUserAdapter::SyncReflectedVarToIVar(IVariable* pVariable)
{
    QString value;
    pVariable->Get(value);
    m_reflectedVar->m_value = value.toUtf8().data();

    // extract the list of custom items from the IVariable user data
    IVariable::IGetCustomItems* pGetCustomItems = static_cast<IVariable::IGetCustomItems*>(pVariable->GetUserData().value<void*>());
    if (pGetCustomItems == nullptr)
    {
        m_reflectedVar->m_enableEdit = false;
        return;
    }

    std::vector<IVariable::IGetCustomItems::SItem> items;
    QString dlgTitle;
    // call the user supplied callback to fill-in items and get dialog title
    bool bShowIt = pGetCustomItems->GetItems(pVariable, items, dlgTitle);
    if (!bShowIt) // if func vetoed it, don't show the dialog
    {
        return;
    }
    m_reflectedVar->m_enableEdit = true;
    m_reflectedVar->m_useTree = pGetCustomItems->UseTree();
    m_reflectedVar->m_treeSeparator = pGetCustomItems->GetTreeSeparator();
    m_reflectedVar->m_dialogTitle = dlgTitle.toUtf8().data();
    m_reflectedVar->m_itemNames.resize(items.size());
    m_reflectedVar->m_itemDescriptions.resize(items.size());

    QByteArray ba;
    int i = -1;
    AZStd::generate(
        m_reflectedVar->m_itemNames.begin(), m_reflectedVar->m_itemNames.end(),
        [&items, &i, &ba]()
        {
            ++i;
            ba = items[i].name.toUtf8();
            return ba.data();
        });
    i = -1;
    AZStd::generate(
        m_reflectedVar->m_itemDescriptions.begin(), m_reflectedVar->m_itemDescriptions.end(),
        [&items, &i, &ba]()
        {
            ++i;
            ba = items[i].desc.toUtf8();
            return ba.data();
        });
}

void ReflectedVarUserAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->Set(m_reflectedVar->m_value.c_str());
}


ReflectedVarSplineAdapter::ReflectedVarSplineAdapter(ReflectedPropertyItem *parentItem, PropertyType propertyType)
    : m_propertyType(propertyType)
    , m_bDontSendToControl(false)
    , m_parentItem(parentItem)
{
}

void ReflectedVarSplineAdapter::SetVariable(IVariable* pVariable)
{
    m_reflectedVar.reset(new CReflectedVarSpline(m_propertyType, pVariable->GetHumanName().toUtf8().data()));

}

void ReflectedVarSplineAdapter::SyncReflectedVarToIVar(IVariable* pVariable)
{
    if (!m_bDontSendToControl)
    {
        m_reflectedVar->m_spline = reinterpret_cast<uint64_t>(pVariable->GetSpline());
    }
}


void ReflectedVarSplineAdapter::SyncIVarToReflectedVar(IVariable* pVariable)
{
    // Splines update variables directly so don't call OnVariableChange or SetValue here or values will be overwritten.

    // Call OnSetValue to force this field to notify this variable that its model has changed without going through the
    // full OnVariableChange pass
    //
    // Set m_bDontSendToControl to prevent the control's data from being overwritten (as the variable's data won't
    // necessarily be up to date vs the controls at the point this happens).
    m_bDontSendToControl = true;
    pVariable->OnSetValue(false);
    m_bDontSendToControl = false;

    m_parentItem->SendOnItemChange();
}

void ReflectedVarMotionAdapter::SetVariable(IVariable *pVariable)
{
    // Create new reflected var
    m_reflectedVar.reset(new CReflectedVarMotion(pVariable->GetHumanName().toLatin1().data()));
    m_reflectedVar->m_description = pVariable->GetDescription().toLatin1().data();

    // Set the asset id
    AZStd::string stringGuid = pVariable->GetDisplayValue().toLatin1().data();
    AZ::Uuid guid(stringGuid.c_str(), stringGuid.length());
    AZ::u32 subId = pVariable->GetUserData().value<AZ::u32>();
    m_reflectedVar->m_assetId = AZ::Data::AssetId(guid, subId);

    // Lookup Filename by assetId and get the filename part of the description
    EBUS_EVENT_RESULT(m_reflectedVar->m_motion, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, m_reflectedVar->m_assetId);
}

void ReflectedVarMotionAdapter::SyncReflectedVarToIVar(IVariable *pVariable)
{
    AZStd::string stringGuid = pVariable->GetDisplayValue().toLatin1().data();
    AZ::Uuid guid(stringGuid.c_str(), stringGuid.length());
    AZ::u32 subId = pVariable->GetUserData().value<AZ::u32>();
    m_reflectedVar->m_assetId = AZ::Data::AssetId(guid, subId);

    // Lookup Filename by assetId and get the filename part of the description
    EBUS_EVENT_RESULT(m_reflectedVar->m_motion, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, m_reflectedVar->m_assetId);
}

void ReflectedVarMotionAdapter::SyncIVarToReflectedVar(IVariable *pVariable)
{
    pVariable->SetUserData(m_reflectedVar->m_assetId.m_subId);
    pVariable->SetDisplayValue(m_reflectedVar->m_assetId.m_guid.ToString<AZStd::string>().c_str());
}
