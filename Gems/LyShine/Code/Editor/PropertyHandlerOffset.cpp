/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerOffset.h"

#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutFitterBus.h>

#include <QLabel>

Q_DECLARE_METATYPE(UiTransform2dInterface::Anchors);

void PropertyHandlerOffset::ConsumeAttribute(AzQtComponents::VectorInput* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    UIVectorPropertyHandlerBase::ConsumeAttribute(GUI, attrib, attrValue, debugName);

    if (attrib == AZ_CRC_CE("LayoutFitterType"))
    {
        UiLayoutFitterInterface::FitType fitType = UiLayoutFitterInterface::FitType::None;
        if (attrValue->Read<UiLayoutFitterInterface::FitType>(fitType))
        {
            bool horizFit = (fitType == UiLayoutFitterInterface::FitType::HorizontalAndVertical || fitType == UiLayoutFitterInterface::FitType::HorizontalOnly);
            bool vertFit = (fitType == UiLayoutFitterInterface::FitType::HorizontalAndVertical || fitType == UiLayoutFitterInterface::FitType::VerticalOnly);

            AzQtComponents::VectorElement** elements = GUI->getElements();

            elements[2]->getSpinBox()->setEnabled(!horizFit);
            elements[3]->getSpinBox()->setEnabled(!vertFit);
        }
        else
        {
            // emit a warning!
            AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'LayoutFitterType' attribute from property '%s' into string box", debugName);
        }
    }
}

void PropertyHandlerOffset::WriteGUIValuesIntoProperty(size_t index, AzQtComponents::VectorInput* GUI, UiTransform2dInterface::Offsets& instance, AzToolsFramework::InstanceDataNode* node)
{
    AZ::EntityId id = GetParentEntityId(node, index);

    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, id, &UiTransform2dBus::Events::GetAnchors);

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, id, &UiTransformBus::Events::GetPivot);

    AZStd::string labels[4];
    GetLabels(anchors, labels);

    AzQtComponents::VectorElement** elements = GUI->getElements();

    UiTransform2dInterface::Offsets guiDisplayedOffset = ExtractValuesFromGUI(GUI);

    // Set the new display offsets for the element being edited
    UiTransform2dInterface::Offsets newDisplayedOffset = InternalOffsetToDisplayedOffset(instance, anchors, pivot);
    int idx = 0;
    if (elements[idx]->wasValueEditedByUser())
    {
        QLabel* label = elements[idx]->getLabelWidget();
        if (label && (label->text() == labels[idx].c_str()))
        {
            newDisplayedOffset.m_left = guiDisplayedOffset.m_left;
        }
    }
    idx++;
    if (elements[idx]->wasValueEditedByUser())
    {
        QLabel* label = elements[idx]->getLabelWidget();
        if (label && (label->text() == labels[idx].c_str()))
        {
            newDisplayedOffset.m_top = guiDisplayedOffset.m_top;
        }
    }
    idx++;
    if (elements[idx]->wasValueEditedByUser())
    {
        QLabel* label = elements[idx]->getLabelWidget();
        if (label && (label->text() == labels[idx].c_str()))
        {
            newDisplayedOffset.m_right = guiDisplayedOffset.m_right;
        }
    }
    idx++;
    if (elements[idx]->wasValueEditedByUser())
    {
        QLabel* label = elements[idx]->getLabelWidget();
        if (label && (label->text() == labels[idx].c_str()))
        {
            newDisplayedOffset.m_bottom = guiDisplayedOffset.m_bottom;
        }
    }

    UiTransform2dInterface::Offsets newInternalOffset;
    newInternalOffset = DisplayedOffsetToInternalOffset(newDisplayedOffset, anchors, pivot);

    // IMPORTANT: This will indirectly update "instance".
    UiTransform2dBus::Event(id, &UiTransform2dBus::Events::SetOffsets, newInternalOffset);
}

bool PropertyHandlerOffset::ReadValuesIntoGUI([[maybe_unused]] size_t index, AzQtComponents::VectorInput* GUI, const UiTransform2dInterface::Offsets& instance, AzToolsFramework::InstanceDataNode* node)
{
    // IMPORTANT: We DON'T need to do validation of data here because that's
    // done for us BEFORE we get here. We DO need to set the labels here.

    AZ::EntityId id = GetParentEntityId(node, index);

    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, id, &UiTransform2dBus::Events::GetAnchors);

    // Set the labels.
    {
        SetLabels(GUI, anchors);
    }

    GUI->blockSignals(true);
    {
        AZ::Vector2 pivot;
        UiTransformBus::EventResult(pivot, id, &UiTransformBus::Events::GetPivot);

        UiTransform2dInterface::Offsets displayedOffset = InternalOffsetToDisplayedOffset(instance, anchors, pivot);
        InsertValuesIntoGUI(GUI, displayedOffset);
    }
    GUI->blockSignals(false);

    return false;
}

void PropertyHandlerOffset::GetLabels(UiTransform2dInterface::Anchors& anchors, AZStd::string* labelsOut)
{
    labelsOut[0] = "Left";
    labelsOut[1] = "Top";
    labelsOut[2] = "Right";
    labelsOut[3] = "Bottom";

    // If the left and right anchors are the same, allow editing x position and width
    if (anchors.m_left == anchors.m_right)
    {
        labelsOut[0] = "X Pos";
        labelsOut[2] = "Width";
    }

    // If the top and bottom anchors are the same, allow editing y position and height
    if (anchors.m_top == anchors.m_bottom)
    {
        labelsOut[1] = "Y Pos";
        labelsOut[3] = "Height";
    }
}

void PropertyHandlerOffset::SetLabels(AzQtComponents::VectorInput* ctrl,
    UiTransform2dInterface::Anchors& anchors)
{
    AZStd::string labels[4];
    GetLabels(anchors, labels);

    for (int i = 0; i < 4; i++)
    {
        ctrl->setLabel(i, labels[i].c_str());
    }
}

AZ::EntityId PropertyHandlerOffset::GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index)
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

UiTransform2dInterface::Offsets PropertyHandlerOffset::InternalOffsetToDisplayedOffset(UiTransform2dInterface::Offsets internalOffset,
    const UiTransform2dInterface::Anchors& anchors,
    const AZ::Vector2& pivot)
{
    // This is complex because the X offsets can be displayed
    // as either left & right or as xpos & width and the Y offsets can be displayed
    // as either top & bottom or as ypos and height.

    UiTransform2dInterface::Offsets displayedOffset = internalOffset;

    // If the left and right anchors are the same, allow editing x position and width
    if (anchors.m_left == anchors.m_right)
    {
        float width = internalOffset.m_right - internalOffset.m_left;

        // width
        displayedOffset.m_right = width;

        // X Pos
        displayedOffset.m_left = internalOffset.m_left + pivot.GetX() * width;
    }

    // If the top and bottom anchors are the same, allow editing y position and height
    if (anchors.m_top == anchors.m_bottom)
    {
        float height = internalOffset.m_bottom - internalOffset.m_top;

        // height
        displayedOffset.m_bottom = height;

        // Y Pos
        displayedOffset.m_top = internalOffset.m_top + pivot.GetY() * height;
    }

    return displayedOffset;
}

UiTransform2dInterface::Offsets PropertyHandlerOffset::DisplayedOffsetToInternalOffset(UiTransform2dInterface::Offsets displayedOffset,
    const UiTransform2dInterface::Anchors& anchors,
    const AZ::Vector2& pivot)
{
    UiTransform2dInterface::Offsets internalOffset = displayedOffset;

    if (anchors.m_left == anchors.m_right)
    {
        // flipping of offsets is not allowed, so if width is negative make it zero
        float xPos = displayedOffset.m_left;
        float width = AZ::GetMax(0.0f, displayedOffset.m_right);

        internalOffset.m_left = xPos - pivot.GetX() * width;
        internalOffset.m_right = internalOffset.m_left + width;
    }

    if (anchors.m_top == anchors.m_bottom)
    {
        // flipping of offsets is not allowed, so if height is negative make it zero
        float yPos = displayedOffset.m_top;
        float height = AZ::GetMax(0.0f, displayedOffset.m_bottom);

        internalOffset.m_top = yPos - pivot.GetY() * height;
        internalOffset.m_bottom = internalOffset.m_top + height;
    }

    return internalOffset;
}

void PropertyHandlerOffset::Register()
{
    qRegisterMetaType<UiTransform2dInterface::Anchors>("UiTransform2dInterface::Anchors");
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerOffset());
}
