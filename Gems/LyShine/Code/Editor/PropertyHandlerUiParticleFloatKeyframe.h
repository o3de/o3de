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
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyColorCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyDoubleSpinCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEnumComboBoxCtrl.hxx>

#include <LyShine/Bus/UiParticleEmitterBus.h>
#endif

class PropertyUiParticleFloatKeyframeCtrl
    : public QWidget
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(PropertyUiParticleFloatKeyframeCtrl, AZ::SystemAllocator, 0);

    PropertyUiParticleFloatKeyframeCtrl(QWidget* parent = nullptr);

    void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName);

    AzToolsFramework::PropertyDoubleSpinCtrl* GetTimeCtrl();
    AzToolsFramework::PropertyDoubleSpinCtrl* GetMultiplierCtrl();
    AzToolsFramework::PropertyEnumComboBoxCtrl* GetInTangentCtrl();
    AzToolsFramework::PropertyEnumComboBoxCtrl* GetOutTangentCtrl();

private:

    AzToolsFramework::PropertyDoubleSpinCtrl* m_timeCtrl;
    AzToolsFramework::PropertyDoubleSpinCtrl* m_multiplierCtrl;
    AzToolsFramework::PropertyEnumComboBoxCtrl* m_inTangentCtrl;
    AzToolsFramework::PropertyEnumComboBoxCtrl* m_outTangentCtrl;
};

class PropertyHandlerUiParticleFloatKeyframe
    : public AzToolsFramework::PropertyHandler < UiParticleEmitterInterface::ParticleFloatKeyframe, PropertyUiParticleFloatKeyframeCtrl >
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerUiParticleFloatKeyframe, AZ::SystemAllocator, 0);

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC("UiParticleFloatKeyframeCtrl", 0xba9359a2); }
    bool IsDefaultHandler() const override { return true; }
    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(PropertyUiParticleFloatKeyframeCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, PropertyUiParticleFloatKeyframeCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, PropertyUiParticleFloatKeyframeCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    AZ::EntityId GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index);

    static void Register();
};
