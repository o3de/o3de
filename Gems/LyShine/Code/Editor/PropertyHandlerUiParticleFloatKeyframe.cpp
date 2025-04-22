/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerUiParticleFloatKeyframe.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>

PropertyUiParticleFloatKeyframeCtrl::PropertyUiParticleFloatKeyframeCtrl(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 5, 0, 5);
    vLayout->setSpacing(2);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    QHBoxLayout* layoutRow2 = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    QLabel* timeLabel = new QLabel(parent);
    timeLabel->setText("Time");
    timeLabel->setObjectName("Time");
    layout->addWidget(timeLabel);

    m_timeCtrl = aznew AzToolsFramework::PropertyDoubleSpinCtrl(parent);
    m_timeCtrl->setMinimum(0.0f);
    m_timeCtrl->setMaximum(1.0f);
    m_timeCtrl->setStep(0.0f);
    m_timeCtrl->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
    m_timeCtrl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_timeCtrl->setToolTip(tr("Time in the range [0,1]."));

    QObject::connect(m_timeCtrl, &AzToolsFramework::PropertyDoubleSpinCtrl::valueChanged, this, [this]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
        });

    layout->addWidget(m_timeCtrl);

    QLabel* multiplierLabel = new QLabel(parent);
    multiplierLabel->setText("Multiplier");
    multiplierLabel->setObjectName("Multiplier");
    layout->addWidget(multiplierLabel);

    m_multiplierCtrl = aznew AzToolsFramework::PropertyDoubleSpinCtrl(parent);
    m_multiplierCtrl->setMinimum(-100.0f);
    m_multiplierCtrl->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
    m_multiplierCtrl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    QObject::connect(m_multiplierCtrl, &AzToolsFramework::PropertyDoubleSpinCtrl::valueChanged, this, [this]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
        });

    layout->addWidget(m_multiplierCtrl);

    AZStd::pair<AZ::s64, AZStd::string> easeInTangent;
    easeInTangent.first = static_cast<AZ::s64>(UiParticleEmitterInterface::ParticleKeyframeTangentType::EaseIn);
    easeInTangent.second = "Ease In";
    AZStd::pair<AZ::s64, AZStd::string> easeOutTangent;
    easeOutTangent.first = static_cast<AZ::s64>(UiParticleEmitterInterface::ParticleKeyframeTangentType::EaseOut);
    easeOutTangent.second = "Ease Out";
    AZStd::pair<AZ::s64, AZStd::string> linearTangent;
    linearTangent.first = static_cast<AZ::s64>(UiParticleEmitterInterface::ParticleKeyframeTangentType::Linear);
    linearTangent.second = "Linear";
    AZStd::pair<AZ::s64, AZStd::string> stepTangent;
    stepTangent.first = static_cast<AZ::s64>(UiParticleEmitterInterface::ParticleKeyframeTangentType::Step);
    stepTangent.second = "Step";

    QLabel* inTangentLabel = new QLabel(parent);
    inTangentLabel->setText("In tangent");
    inTangentLabel->setObjectName("In tangent");
    layoutRow2->addWidget(inTangentLabel);

    m_inTangentCtrl = aznew AzToolsFramework::PropertyEnumComboBoxCtrl(parent);
    m_inTangentCtrl->addEnumValue(easeInTangent);
    m_inTangentCtrl->addEnumValue(linearTangent);
    m_inTangentCtrl->addEnumValue(stepTangent);

    QObject::connect(m_inTangentCtrl, &AzToolsFramework::PropertyEnumComboBoxCtrl::valueChanged, this, [this]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
        });

    layoutRow2->addWidget(m_inTangentCtrl);

    QLabel* outTangentLabel = new QLabel(parent);
    outTangentLabel->setText("Out tangent");
    outTangentLabel->setObjectName("Out tangent");
    layoutRow2->addWidget(outTangentLabel);

    m_outTangentCtrl = aznew AzToolsFramework::PropertyEnumComboBoxCtrl(parent);
    m_outTangentCtrl->addEnumValue(easeOutTangent);
    m_outTangentCtrl->addEnumValue(linearTangent);
    m_outTangentCtrl->addEnumValue(stepTangent);

    QObject::connect(m_outTangentCtrl, &AzToolsFramework::PropertyEnumComboBoxCtrl::valueChanged, this, [this]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
        });

    layoutRow2->addWidget(m_outTangentCtrl);

    vLayout->addLayout(layout);
    vLayout->addLayout(layoutRow2);
}

void PropertyUiParticleFloatKeyframeCtrl::ConsumeAttribute(AZ::u32 /*attrib*/, AzToolsFramework::PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
{
}

AzToolsFramework::PropertyDoubleSpinCtrl* PropertyUiParticleFloatKeyframeCtrl::GetTimeCtrl()
{
    return m_timeCtrl;
}

AzToolsFramework::PropertyDoubleSpinCtrl* PropertyUiParticleFloatKeyframeCtrl::GetMultiplierCtrl()
{
    return m_multiplierCtrl;
}

AzToolsFramework::PropertyEnumComboBoxCtrl* PropertyUiParticleFloatKeyframeCtrl::GetInTangentCtrl()
{
    return m_inTangentCtrl;
}

AzToolsFramework::PropertyEnumComboBoxCtrl* PropertyUiParticleFloatKeyframeCtrl::GetOutTangentCtrl()
{
    return m_outTangentCtrl;
}

QWidget* PropertyHandlerUiParticleFloatKeyframe::CreateGUI(QWidget* pParent)
{
    return aznew PropertyUiParticleFloatKeyframeCtrl(pParent);
}

void PropertyHandlerUiParticleFloatKeyframe::ConsumeAttribute(PropertyUiParticleFloatKeyframeCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    GUI->ConsumeAttribute(attrib, attrValue, debugName);
}

void PropertyHandlerUiParticleFloatKeyframe::WriteGUIValuesIntoProperty(size_t /*index*/, PropertyUiParticleFloatKeyframeCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
{
    instance.time = aznumeric_cast<float>(GUI->GetTimeCtrl()->value());
    instance.multiplier = aznumeric_cast<float>(GUI->GetMultiplierCtrl()->value());
    instance.inTangent = static_cast<UiParticleEmitterInterface::ParticleKeyframeTangentType>(GUI->GetInTangentCtrl()->value());
    instance.outTangent = static_cast<UiParticleEmitterInterface::ParticleKeyframeTangentType>(GUI->GetOutTangentCtrl()->value());
}

bool PropertyHandlerUiParticleFloatKeyframe::ReadValuesIntoGUI(size_t /*index*/, PropertyUiParticleFloatKeyframeCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
{
    GUI->blockSignals(true);
    {
        GUI->GetTimeCtrl()->setValue(instance.time);
        GUI->GetMultiplierCtrl()->setValue(instance.multiplier);
        GUI->GetInTangentCtrl()->setValue(static_cast<AZ::s64>(instance.inTangent));
        GUI->GetOutTangentCtrl()->setValue(static_cast<AZ::s64>(instance.outTangent));
    }
    GUI->blockSignals(false);

    return false;
}

AZ::EntityId PropertyHandlerUiParticleFloatKeyframe::GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index)
{
    while (node)
    {
        if ((node->GetClassMetadata()) && (node->GetClassMetadata()->m_azRtti))
        {
            if (node->GetClassMetadata()->m_azRtti->IsTypeOf(AZ::Component::RTTI_Type()))
            {
                return static_cast<AZ::Component*>(node->GetInstance(index))->GetEntityId();
            }
        }

        node = node->GetParent();
    }

    return AZ::EntityId();
}

void PropertyHandlerUiParticleFloatKeyframe::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType,
        aznew PropertyHandlerUiParticleFloatKeyframe());
}

#include <moc_PropertyHandlerUiParticleFloatKeyframe.cpp>
