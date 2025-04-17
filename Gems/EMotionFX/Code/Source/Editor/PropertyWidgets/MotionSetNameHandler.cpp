/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionSetNameHandler.h"
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/System/SystemCommon.h>
#include <EMotionFX/Source/MotionSet.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionSetNameHandler, EditorAllocator)

    AZ::u32 MotionSetNameHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("MotionSetName");
    }


    QWidget* MotionSetNameHandler::CreateGUI(QWidget* parent)
    {
        QComboBox* picker = new QComboBox(parent);

        connect(picker, &QComboBox::currentTextChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void MotionSetNameHandler::ConsumeAttribute(QComboBox* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
        else if (attrib == AZ_CRC_CE("MotionSetAsset"))
        {
            AZ::Data::Asset<Integration::MotionSetAsset>* value;
            if (attrValue->Read<AZ::Data::Asset<Integration::MotionSetAsset>*>(value))
            {
                m_motionSetAsset = value;
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("MotionSetNameHandler", false, "Failed to read 'MotionSetAsset' attribute from property '%s' into MotionSetNameHandler", debugName);
            }
        }
    }


    void MotionSetNameHandler::WriteGUIValuesIntoProperty(size_t index, QComboBox* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        const QString& currentText = GUI->currentText();
        instance = AZStd::string(currentText.toUtf8().data(), currentText.length());
    }


    bool MotionSetNameHandler::ReadValuesIntoGUI(size_t index, QComboBox* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        QSignalBlocker signalBlocker(GUI);
        GUI->clear();
        if (m_motionSetAsset && m_motionSetAsset->Get() && m_motionSetAsset->Get()->m_emfxMotionSet)
        {
            const AZStd::unique_ptr<EMotionFX::MotionSet>& emfxMotionSet = m_motionSetAsset->Get()->m_emfxMotionSet;
            AZStd::vector<const MotionSet*> motionSets;
            const bool isOwnedByRutime = emfxMotionSet->GetIsOwnedByRuntime();
            emfxMotionSet->RecursiveGetMotionSets(motionSets, isOwnedByRutime);

            for (const EMotionFX::MotionSet* motionSet : motionSets)
            {
                GUI->addItem(motionSet->GetName());
            }
            if (!instance.empty())
            {
                GUI->setCurrentText(instance.c_str());
            }
            else 
            {
                // Choose the root motion set
                GUI->setCurrentText(emfxMotionSet->GetName());
            }
        }
        else if (!GUI->isEnabled() && !instance.empty())
        {
            // When the game is running, the handler is disabled but we still want to show the value
            GUI->addItem(instance.c_str());
            GUI->setCurrentText(instance.c_str());
        }
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_MotionSetNameHandler.cpp>
