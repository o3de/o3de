/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    AZ_CLASS_ALLOCATOR(PropertyUiParticleFloatKeyframeCtrl, AZ::SystemAllocator);

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
    AZ_CLASS_ALLOCATOR(PropertyHandlerUiParticleFloatKeyframe, AZ::SystemAllocator);

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("UiParticleFloatKeyframeCtrl"); }
    bool IsDefaultHandler() const override { return true; }
    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(PropertyUiParticleFloatKeyframeCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, PropertyUiParticleFloatKeyframeCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, PropertyUiParticleFloatKeyframeCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    AZ::EntityId GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index);

    static void Register();
};
