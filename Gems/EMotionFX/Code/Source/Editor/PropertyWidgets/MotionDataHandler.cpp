/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionDataHandler.h"
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/System/SystemCommon.h>
#include <EMotionFX/Source/MotionData/MotionDataFactory.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionDataHandler, EditorAllocator)

    AZ::u32 MotionDataHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("MotionData");
    }

    QWidget* MotionDataHandler::CreateGUI(QWidget* parent)
    {
        QComboBox* picker = new QComboBox(parent);

        connect(picker, &QComboBox::currentTextChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }

    void MotionDataHandler::ConsumeAttribute(QComboBox* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void MotionDataHandler::WriteGUIValuesIntoProperty(size_t index, QComboBox* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        const int currentIndex = GUI->currentIndex();
        if (currentIndex == -1)
        {
            instance = m_typeIds[0];
        }
        else
        {
            instance = m_typeIds[currentIndex];
        }
    }

    bool MotionDataHandler::ReadValuesIntoGUI(size_t index, QComboBox* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        QSignalBlocker signalBlocker(GUI);
        GUI->clear();
        m_typeIds.clear();

        GUI->addItem("Automatic (prefer performance if within memory limits)");
        m_typeIds.emplace_back(AZ::TypeId::CreateNull());

        const MotionDataFactory& factory = GetEMotionFX().GetMotionManager()->GetMotionDataFactory();
        for (size_t i = 0; i < factory.GetNumRegistered(); ++i)
        {
            GUI->addItem(factory.GetRegistered(i)->GetSceneSettingsName());
            m_typeIds.emplace_back(factory.GetRegistered(i)->RTTI_GetType());
        }

        if (instance.IsNull())
        {
            GUI->setCurrentIndex(0);
        }
        else 
        {
            AZ::Outcome<size_t> motionIndex = factory.FindRegisteredIndexByTypeId(instance);
            if (motionIndex.IsSuccess())
            {
                GUI->setCurrentIndex(static_cast<int>(motionIndex.GetValue() + 1)); // +1 because we inserted an 'Automatic' one as first entry.
            }
            else
            {
                AZ_Warning("EMotionFX", false, "MotionData handler can't find the motion data with typeId '%s', selecting 'Automatic' instead", instance.ToString<AZStd::string>().c_str());
                GUI->setCurrentIndex(0);
            }
        }

        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_MotionDataHandler.cpp>
