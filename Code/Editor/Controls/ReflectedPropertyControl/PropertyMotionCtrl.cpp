/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "PropertyMotionCtrl.h"

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

QWidget* MotionPropertyWidgetHandler::CreateGUI(QWidget* pParent)
{
    AzToolsFramework::PropertyAssetCtrl* newCtrl = aznew AzToolsFramework::PropertyAssetCtrl(pParent);
    connect(
        newCtrl, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl]([[maybe_unused]] AZ::Data::AssetId newAssetId) {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
    return newCtrl;
}

void MotionPropertyWidgetHandler::ConsumeAttribute(
    [[maybe_unused]] AzToolsFramework::PropertyAssetCtrl* GUI, [[maybe_unused]] AZ::u32 attrib,
    [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
{
}

void MotionPropertyWidgetHandler::WriteGUIValuesIntoProperty(
    [[maybe_unused]] size_t index, [[maybe_unused]] AzToolsFramework::PropertyAssetCtrl* GUI, property_t& instance,
    [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    CReflectedVarMotion val;
    val.m_motion = GUI->GetCurrentAssetHint();
    val.m_assetId = GUI->GetSelectedAssetID();
    instance = static_cast<property_t>(val);
}

bool MotionPropertyWidgetHandler::ReadValuesIntoGUI(
    [[maybe_unused]] size_t index, [[maybe_unused]] AzToolsFramework::PropertyAssetCtrl* GUI, const property_t& instance,
    [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    static const AZ::Data::AssetType emotionFXMotionAssetType(
        "{00494B8E-7578-4BA2-8B28-272E90680787}"); // from MotionAsset.h in EMotionFX Gem

    GUI->blockSignals(true);
    GUI->SetSelectedAssetID(instance.m_assetId);
    GUI->SetCurrentAssetType(emotionFXMotionAssetType);
    GUI->blockSignals(false);

    return false;
}

#include <Controls/ReflectedPropertyControl/moc_PropertyMotionCtrl.cpp>
