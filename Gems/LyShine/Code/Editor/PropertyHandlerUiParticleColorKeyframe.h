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

class PropertyUiParticleColorKeyframeCtrl
    : public QWidget
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(PropertyUiParticleColorKeyframeCtrl, AZ::SystemAllocator);

    PropertyUiParticleColorKeyframeCtrl(QWidget* parent = nullptr);

    void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName);

    AzToolsFramework::PropertyDoubleSpinCtrl* GetTimeCtrl();
    AzToolsFramework::PropertyColorCtrl* GetColorCtrl();
    AzToolsFramework::PropertyEnumComboBoxCtrl* GetInTangentCtrl();
    AzToolsFramework::PropertyEnumComboBoxCtrl* GetOutTangentCtrl();

private:

    AzToolsFramework::PropertyDoubleSpinCtrl* m_timeCtrl;
    AzToolsFramework::PropertyColorCtrl* m_colorCtrl;
    AzToolsFramework::PropertyEnumComboBoxCtrl* m_inTangentCtrl;
    AzToolsFramework::PropertyEnumComboBoxCtrl* m_outTangentCtrl;
};

class PropertyHandlerUiParticleColorKeyframe
    : public AzToolsFramework::PropertyHandler < UiParticleEmitterInterface::ParticleColorKeyframe, PropertyUiParticleColorKeyframeCtrl >
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerUiParticleColorKeyframe, AZ::SystemAllocator);

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("UiParticleColorKeyframeCtrl"); }
    bool IsDefaultHandler() const override { return true; }
    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(PropertyUiParticleColorKeyframeCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, PropertyUiParticleColorKeyframeCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, PropertyUiParticleColorKeyframeCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    AZ::EntityId GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index);

    static void Register();
};
