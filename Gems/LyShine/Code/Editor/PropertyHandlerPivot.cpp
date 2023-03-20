/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerPivot.h"

#include "PivotPresets.h"
#include "PivotPresetsWidget.h"

#include <QBoxLayout>

PropertyPivotCtrl::PropertyPivotCtrl(QWidget* parent)
    : QWidget(parent)
    , m_common(2, 1)
    , m_propertyVectorCtrl(m_common.ConstructGUI(this))
    , m_pivotPresetsWidget(nullptr)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Add Preset buttons.
    {
        AzQtComponents::VectorElement** elements = m_propertyVectorCtrl->getElements();
        AZ::Vector2 controlValue(aznumeric_cast<float>(elements[0]->getValue()), aznumeric_cast<float>(elements[1]->getValue()));

        m_pivotPresetsWidget = new PivotPresetsWidget(PivotPresets::PivotToPresetIndex(controlValue),
                [this](int presetIndex)
                {
                    AZ::Vector2 presetValues = PivotPresets::PresetIndexToPivot(presetIndex);
                    m_propertyVectorCtrl->setValuebyIndex(presetValues.GetX(), 0);
                    m_propertyVectorCtrl->setValuebyIndex(presetValues.GetY(), 1);

                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                        &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
                },
                this);

        layout->addWidget(m_pivotPresetsWidget);
    }

    // Vector ctrl.
    {
        m_propertyVectorCtrl->setLabel(0, "X");
        m_propertyVectorCtrl->setLabel(1, "Y");

        QObject::connect(m_propertyVectorCtrl, &AzQtComponents::VectorInput::valueChanged, this, [this]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
            });

        m_propertyVectorCtrl->setMinimum(-std::numeric_limits<float>::max());
        m_propertyVectorCtrl->setMaximum(std::numeric_limits<float>::max());

        layout->addWidget(m_propertyVectorCtrl);
    }
}

void PropertyPivotCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    m_common.ConsumeAttributes(GetPropertyVectorCtrl(), attrib, attrValue, debugName);
}

PivotPresetsWidget* PropertyPivotCtrl::GetPivotPresetsWidget()
{
    return m_pivotPresetsWidget;
}

AzQtComponents::VectorInput* PropertyPivotCtrl::GetPropertyVectorCtrl()
{
    return m_propertyVectorCtrl;
}

//-------------------------------------------------------------------------------

QWidget* PropertyHandlerPivot::CreateGUI(QWidget* pParent)
{
    return aznew PropertyPivotCtrl(pParent);
}

void PropertyHandlerPivot::ConsumeAttribute(PropertyPivotCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    GUI->ConsumeAttribute(attrib, attrValue, debugName);
}

void PropertyHandlerPivot::WriteGUIValuesIntoProperty(size_t index, PropertyPivotCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    AzQtComponents::VectorElement** elements = GUI->GetPropertyVectorCtrl()->getElements();

    // Check if a pivot preset has been selected
    bool presetSelected = true;
    for (int idx = 0; idx < GUI->GetPropertyVectorCtrl()->getSize(); ++idx)
    {
        if (elements[idx]->wasValueEditedByUser())
        {
            presetSelected = false;
            break;
        }
    }

    AZ::Vector2 newPivot;
    if (presetSelected)
    {
        newPivot.SetX(aznumeric_cast<float>(elements[0]->getValue()));
        newPivot.SetY(aznumeric_cast<float>(elements[1]->getValue()));
    }
    else
    {
        newPivot = instance;

        if (elements[0]->wasValueEditedByUser())
        {
            newPivot.SetX(aznumeric_cast<float>(elements[0]->getValue()));
        }
        if (elements[1]->wasValueEditedByUser())
        {
            newPivot.SetY(aznumeric_cast<float>(elements[1]->getValue()));
        }
    }

    // Check if this element is being controlled by its parent
    AZ::EntityId entityId = GetParentEntityId(node, index);
    bool isControlledByParent = false;
    AZ::Entity* parentElement = nullptr;
    UiElementBus::EventResult(parentElement, entityId, &UiElementBus::Events::GetParent);
    if (parentElement)
    {
        UiLayoutBus::EventResult(isControlledByParent, parentElement->GetId(), &UiLayoutBus::Events::IsControllingChild, entityId);
    }

    // IMPORTANT: This will indirectly update "instance".
    if (isControlledByParent)
    {
        UiTransformBus::Event(entityId, &UiTransformBus::Events::SetPivot, newPivot);
    }
    else
    {
        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetPivotAndAdjustOffsets, newPivot);
    }
}

bool PropertyHandlerPivot::ReadValuesIntoGUI([[maybe_unused]] size_t index, PropertyPivotCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    AzQtComponents::VectorInput* ctrl = GUI->GetPropertyVectorCtrl();

    ctrl->blockSignals(true);
    {
        ctrl->setValuebyIndex(instance.GetX(), 0);
        ctrl->setValuebyIndex(instance.GetY(), 1);
    }
    ctrl->blockSignals(false);

    GUI->GetPivotPresetsWidget()->SetPresetSelection(PivotPresets::PivotToPresetIndex(instance));

    return false;
}

AZ::EntityId PropertyHandlerPivot::GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index)
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

void PropertyHandlerPivot::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerPivot());
}

#include <moc_PropertyHandlerPivot.cpp>
