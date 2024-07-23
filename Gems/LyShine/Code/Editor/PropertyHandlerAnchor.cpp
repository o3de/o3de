/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerAnchor.h"

#include "AnchorPresets.h"
#include "AnchorPresetsWidget.h"

#include <LyShine/Bus/UiLayoutFitterBus.h>

#include <QBoxLayout>
#include <QLabel>

PropertyAnchorCtrl::PropertyAnchorCtrl(QWidget* parent)
    : QWidget(parent)
    , m_common(4, 1)
    , m_propertyVectorCtrl(m_common.ConstructGUI(this))
    , m_anchorPresetsWidget(nullptr)
    , m_disabledLabel(nullptr)
    , m_controlledByFitterLabel(nullptr)
    , m_isReadOnly(false)
{
    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);

    // Disabled label (used when the property is read-only)
    // This is a special feature of the anchor property - it is used to display a message
    // when the transform is disabled.
    {
        m_disabledLabel = new QLabel(this);
        m_disabledLabel->setText("Anchors and Offsets are\ncontrolled by parent");
        m_disabledLabel->setVisible(false);
        vLayout->addWidget(m_disabledLabel);
    }

    // Controlled by fitter label
    // Used to display a message when the transform is being controlled by a layout fitter
    {
        m_controlledByFitterLabel = new QLabel(this);
        m_controlledByFitterLabel->setText(""); // text depends on fit level
        m_controlledByFitterLabel->setVisible(false);
        vLayout->addWidget(m_controlledByFitterLabel);
    }

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Add Preset buttons.
    {
        AzQtComponents::VectorElement** elements = m_propertyVectorCtrl->getElements();
        AZ::Vector4 controlValue(aznumeric_cast<float>(elements[0]->getValue()), aznumeric_cast<float>(elements[1]->getValue()), aznumeric_cast<float>(elements[2]->getValue()), aznumeric_cast<float>(elements[3]->getValue()));

        m_anchorPresetsWidget = new AnchorPresetsWidget(AnchorPresets::AnchorToPresetIndex(controlValue),
                [this](int presetIndex)
                {
                    AZ::Vector4 presetValues = AnchorPresets::PresetIndexToAnchor(presetIndex) * 100.0f;
                    m_propertyVectorCtrl->setValuebyIndex(presetValues.GetX(), 0);
                    m_propertyVectorCtrl->setValuebyIndex(presetValues.GetY(), 1);
                    m_propertyVectorCtrl->setValuebyIndex(presetValues.GetZ(), 2);
                    m_propertyVectorCtrl->setValuebyIndex(presetValues.GetW(), 3);

                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                        &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
                },
                this);

        layout->addWidget(m_anchorPresetsWidget);
    }

    // Vector ctrl.
    {
        m_propertyVectorCtrl->setLabel(0, "Left");
        m_propertyVectorCtrl->setLabel(1, "Top");
        m_propertyVectorCtrl->setLabel(2, "Right");
        m_propertyVectorCtrl->setLabel(3, "Bottom");

        QObject::connect(m_propertyVectorCtrl, &AzQtComponents::VectorInput::valueChanged, this, [this]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
            });

        m_propertyVectorCtrl->setMinimum(-std::numeric_limits<float>::max());
        m_propertyVectorCtrl->setMaximum(std::numeric_limits<float>::max());

        layout->addWidget(m_propertyVectorCtrl);
    }

    vLayout->addLayout(layout);
}

void PropertyAnchorCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    m_common.ConsumeAttributes(GetPropertyVectorCtrl(), attrib, attrValue, debugName);

    if (attrib == AZ::Edit::Attributes::ReadOnly)
    {
        bool value;
        if (attrValue->Read<bool>(value))
        {
            if (value)
            {
                // the property is disabled so hide the normal widgets and show the disabled widget
                m_anchorPresetsWidget->setVisible(false);
                m_propertyVectorCtrl->setVisible(false);
                m_disabledLabel->setVisible(true);
                m_isReadOnly = true;
            }
        }
        else
        {
            // emit a warning!
            AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'ReadOnly' attribute from property '%s' into string box", debugName);
        }
        return;
    }
    else if (attrib == AZ_CRC_CE("LayoutFitterType"))
    {
        UiLayoutFitterInterface::FitType fitType = UiLayoutFitterInterface::FitType::None;
        if (attrValue->Read<UiLayoutFitterInterface::FitType>(fitType))
        {
            bool horizFit = (fitType == UiLayoutFitterInterface::FitType::HorizontalAndVertical || fitType == UiLayoutFitterInterface::FitType::HorizontalOnly);
            bool vertFit = (fitType == UiLayoutFitterInterface::FitType::HorizontalAndVertical || fitType == UiLayoutFitterInterface::FitType::VerticalOnly);

            // Enable or disable the horizontal stretch anchors (separated) depending on whether
            // horizontal fit is enabled
            m_anchorPresetsWidget->SetPresetButtonEnabledAt(3, !horizFit);
            m_anchorPresetsWidget->SetPresetButtonEnabledAt(7, !horizFit);
            m_anchorPresetsWidget->SetPresetButtonEnabledAt(11, !horizFit);

            // Enable or disable the vertical stretch anchors (separated) depending on whether
            // vertical fit is enabled
            m_anchorPresetsWidget->SetPresetButtonEnabledAt(12, !vertFit);      
            m_anchorPresetsWidget->SetPresetButtonEnabledAt(13, !vertFit);
            m_anchorPresetsWidget->SetPresetButtonEnabledAt(14, !vertFit);

            // Enable or disable the horizontal and vertical stretch anchor depending on whether
            // horizontal and vertical fit is enabled
            m_anchorPresetsWidget->SetPresetButtonEnabledAt(15, !(horizFit || vertFit));

            // Set text describing why some properties are disabled
            const char* controlledByFitterText = nullptr;
            if (fitType == UiLayoutFitterInterface::FitType::HorizontalAndVertical)
            {
                controlledByFitterText = "Element width and height are controlled\nby the layout fitter. The layout fitter\nalso controls the anchors by ensuring\nthey are together";
            }
            else if (fitType == UiLayoutFitterInterface::FitType::HorizontalOnly)
            {
                controlledByFitterText = "Element width is controlled by the\nlayout fitter. The layout fitter also\ncontrols the left and right anchors\nby ensuring they are together";
            }
            else if (fitType == UiLayoutFitterInterface::FitType::VerticalOnly)
            {
                controlledByFitterText = "Element height is controlled by the\nlayout fitter. The layout fitter also\ncontrols the top and bottom anchors\nby ensuring they are together";
            }

            if (controlledByFitterText)
            {
                m_controlledByFitterLabel->setText(controlledByFitterText);
                m_controlledByFitterLabel->setVisible(true);
            }
            else
            {
                m_controlledByFitterLabel->setVisible(false);
            }
        }
        else
        {
            // emit a warning!
            AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'LayoutFitterType' attribute from property '%s' into string box", debugName);
        }
    }
}

AnchorPresetsWidget* PropertyAnchorCtrl::GetAnchorPresetsWidget()
{
    return m_anchorPresetsWidget;
}

AzQtComponents::VectorInput* PropertyAnchorCtrl::GetPropertyVectorCtrl()
{
    return m_propertyVectorCtrl;
}

//-------------------------------------------------------------------------------

QWidget* PropertyHandlerAnchor::CreateGUI(QWidget* pParent)
{
    return aznew PropertyAnchorCtrl(pParent);
}

void PropertyHandlerAnchor::ConsumeAttribute(PropertyAnchorCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    GUI->ConsumeAttribute(attrib, attrValue, debugName);
}

void PropertyHandlerAnchor::WriteGUIValuesIntoProperty(size_t index, PropertyAnchorCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    AzQtComponents::VectorElement** elements = GUI->GetPropertyVectorCtrl()->getElements();

    AZ::EntityId entityId = GetParentEntityId(node, index);

    // Check if an anchor preset has been selected
    bool presetSelected = true;
    for (int idx = 0; idx < GUI->GetPropertyVectorCtrl()->getSize(); ++idx)
    {
        if (elements[idx]->wasValueEditedByUser())
        {
            presetSelected = false;
            break;
        }
    }

    // IMPORTANT: This will indirectly update "instance".

    if (presetSelected)
    {
        // Update anchors and adjust pivot and offsets based on the selected preset

        UiTransform2dInterface::Anchors newAnchors(aznumeric_cast<float>(elements[0]->getValue() / 100.0f),
            aznumeric_cast<float>(elements[1]->getValue() / 100.0f),
            aznumeric_cast<float>(elements[2]->getValue() / 100.0f),
            aznumeric_cast<float>(elements[3]->getValue() / 100.0f));

        // Old width is preserved if new anchor left equals right, old height is preserved if new anchor top equals bottom
        float width = -1.0f;
        float height = -1.0f;
        if ((newAnchors.m_left == newAnchors.m_right) || (newAnchors.m_top == newAnchors.m_bottom))
        {
            bool bNeedWidth = (newAnchors.m_left == newAnchors.m_right);
            bool bNeedHeight = (newAnchors.m_top == newAnchors.m_bottom);

            UiTransform2dInterface::Anchors oldAnchors;
            UiTransform2dBus::EventResult(oldAnchors, entityId, &UiTransform2dBus::Events::GetAnchors);

            UiTransform2dInterface::Offsets oldOffsets;
            UiTransform2dBus::EventResult(oldOffsets, entityId, &UiTransform2dBus::Events::GetOffsets);

            // Calculate width/height from offsets if anchors are the same
            if (bNeedWidth && (oldAnchors.m_left == oldAnchors.m_right))
            {
                width = oldOffsets.m_right - oldOffsets.m_left;
            }
            if (bNeedHeight && (oldAnchors.m_top == oldAnchors.m_bottom))
            {
                height = oldOffsets.m_bottom - oldOffsets.m_top;
            }

            if ((bNeedWidth && width < 0.0f) || (bNeedHeight && height < 0.0f))
            {
                // Calculate width/height from element rect in canvas space
                UiTransformInterface::RectPoints elemRect;
                UiTransformBus::Event(entityId, &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, elemRect);
                AZ::Vector2 size = elemRect.GetAxisAlignedSize();
                if (width < 0.0f)
                {
                    width = size.GetX();
                }
                if (height < 0.0f)
                {
                    height = size.GetY();
                }
            }
        }

        // Set anchors to the selected preset values
        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetAnchors, newAnchors, false, false);

        // Adjust pivot
        AZ::Vector2 currentPivot;
        currentPivot.SetX((newAnchors.m_left == newAnchors.m_right) ? newAnchors.m_left : 0.5f);
        currentPivot.SetY((newAnchors.m_top == newAnchors.m_bottom) ? newAnchors.m_top : 0.5f);
        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetPivotAndAdjustOffsets, currentPivot);

        // Adjust offsets
        UiTransform2dInterface::Offsets newOffsets;
        if (newAnchors.m_left == newAnchors.m_right)
        {
            newOffsets.m_left = -currentPivot.GetX() * width;
            newOffsets.m_right = newOffsets.m_left + width;
        }
        else
        {
            newOffsets.m_left = 0.0f;
            newOffsets.m_right = 0.0f;
        }

        if (newAnchors.m_top == newAnchors.m_bottom)
        {
            newOffsets.m_top = -currentPivot.GetY() * height;
            newOffsets.m_bottom = newOffsets.m_top + height;
        }
        else
        {
            newOffsets.m_top = 0.0f;
            newOffsets.m_bottom = 0.0f;
        }
        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetOffsets, newOffsets);
    }
    else
    {
        UiTransform2dInterface::Anchors newAnchors = instance;

        // Check if transform is controlled by a layout fitter
        bool horizontalFit = false;
        UiLayoutFitterBus::EventResult(horizontalFit, entityId, &UiLayoutFitterBus::Events::GetHorizontalFit);

        bool verticalFit = false;
        UiLayoutFitterBus::EventResult(verticalFit, entityId, &UiLayoutFitterBus::Events::GetVerticalFit);

        if (elements[0]->wasValueEditedByUser())
        {
            newAnchors.m_left = aznumeric_cast<float>(elements[0]->getValue() / 100.0f);
            if (horizontalFit)
            {
                newAnchors.m_right = newAnchors.m_left;
            }
        }
        if (elements[1]->wasValueEditedByUser())
        {
            newAnchors.m_top = aznumeric_cast<float>(elements[1]->getValue() / 100.0);
            if (verticalFit)
            {
                newAnchors.m_bottom = newAnchors.m_top;
            }
        }
        if (elements[2]->wasValueEditedByUser())
        {
            newAnchors.m_right = aznumeric_cast<float>(elements[2]->getValue() / 100.0);
            if (horizontalFit)
            {
                newAnchors.m_left = newAnchors.m_right;
            }
        }
        if (elements[3]->wasValueEditedByUser())
        {
            newAnchors.m_bottom = aznumeric_cast<float>(elements[3]->getValue() / 100.0);
            if (verticalFit)
            {
                newAnchors.m_top = newAnchors.m_bottom;
            }
        }

        UiTransform2dBus::Event(entityId, &UiTransform2dBus::Events::SetAnchors, newAnchors, false, true);
    }
}

bool PropertyHandlerAnchor::ReadValuesIntoGUI([[maybe_unused]] size_t index, PropertyAnchorCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    AzQtComponents::VectorInput* ctrl = GUI->GetPropertyVectorCtrl();

    ctrl->blockSignals(true);
    {
        ctrl->setValuebyIndex(instance.m_left * 100.0f, 0);
        ctrl->setValuebyIndex(instance.m_top * 100.0f, 1);
        ctrl->setValuebyIndex(instance.m_right * 100.0f, 2);
        ctrl->setValuebyIndex(instance.m_bottom * 100.0f, 3);
    }
    ctrl->blockSignals(false);

    GUI->GetAnchorPresetsWidget()->SetPresetSelection(AnchorPresets::AnchorToPresetIndex(AZ::Vector4(instance.m_left, instance.m_top, instance.m_right, instance.m_bottom)));

    return false;
}

bool PropertyHandlerAnchor::ModifyTooltip(QWidget* widget, QString& toolTipString)
{
    // We are using the Anchor property handler as a way to display a message when
    // the transform for an element is disabled. In this case we also want to change the
    // tooltip so that it is not specifically about anchors but is about why the
    // transform component properties are hidden.
    PropertyAnchorCtrl* propertyControl = qobject_cast<PropertyAnchorCtrl*>(widget);
    AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
    if (propertyControl)
    {
        if (propertyControl->IsReadOnly())
        {
            toolTipString = "Anchor and Offset properties are not shown because the parent element\n"
                "has a component that is controlling this element's transform.";
        }
        return true;
    }
    return false;
}

AZ::EntityId PropertyHandlerAnchor::GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index)
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

void PropertyHandlerAnchor::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerAnchor());
}

#include <moc_PropertyHandlerAnchor.cpp>
