/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiLayoutFitterComponent.h"
#include "UiLayoutHelpers.h"

#include <LyShine/Bus/UiLayoutCellBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutFitterComponent::UiLayoutFitterComponent()
    : m_horizontalFit(false)
    , m_verticalFit(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutFitterComponent::~UiLayoutFitterComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::ApplyLayoutWidth()
{
    if (m_horizontalFit)
    {
        float targetWidth = UiLayoutHelpers::GetLayoutElementTargetWidth(GetEntityId());

        // Recalculate the new horizontal offsets using the pivot
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);
        UiTransform2dInterface::Anchors anchors;
        UiTransform2dBus::EventResult(anchors, GetEntityId(), &UiTransform2dBus::Events::GetAnchors);

        // If anchors are separate
        if (anchors.m_left != anchors.m_right)
        {
            // Bring them back together in their midpoint
            float midPoint = (anchors.m_left + anchors.m_right) / 2.0f;
            anchors.m_left = anchors.m_right = midPoint;

            UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetAnchors, anchors, false, true);
        }

        float oldWidth = offsets.m_right - offsets.m_left;
        float widthDiff = targetWidth - oldWidth;

        if (widthDiff != 0.0f)
        {
            AZ::Vector2 pivot;
            UiTransformBus::EventResult(pivot, GetEntityId(), &UiTransformBus::Events::GetPivot);

            offsets.m_left -= widthDiff * pivot.GetX();
            offsets.m_right += widthDiff * (1.0f - pivot.GetX());

            UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, offsets);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::ApplyLayoutHeight()
{
    if (m_verticalFit)
    {
        float targetHeight = UiLayoutHelpers::GetLayoutElementTargetHeight(GetEntityId());

        // Recalculate the new vertical offsets using the pivot
        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);
        UiTransform2dInterface::Anchors anchors;
        UiTransform2dBus::EventResult(anchors, GetEntityId(), &UiTransform2dBus::Events::GetAnchors);

        // If anchors are separate
        if (anchors.m_top != anchors.m_bottom)
        {
            // Bring them back together in their midpoint
            float midPoint = (anchors.m_top + anchors.m_bottom) / 2.0f;
            anchors.m_top = anchors.m_bottom = midPoint;

            UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetAnchors, anchors, false, true);
        }

        float oldHeight = offsets.m_bottom - offsets.m_top;
        float heightDiff = targetHeight - oldHeight;

        if (heightDiff != 0.0f)
        {
            AZ::Vector2 pivot;
            UiTransformBus::EventResult(pivot, GetEntityId(), &UiTransformBus::Events::GetPivot);

            offsets.m_top -= heightDiff * pivot.GetY();
            offsets.m_bottom += heightDiff * (1.0f - pivot.GetY());

            UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, offsets);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutFitterComponent::GetHorizontalFit()
{
    return m_horizontalFit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::SetHorizontalFit(bool horizontalFit)
{
    m_horizontalFit = horizontalFit;

    CheckFitterAndInvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutFitterComponent::GetVerticalFit()
{
    return m_verticalFit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::SetVerticalFit(bool verticalFit)
{
    m_verticalFit = verticalFit;

    CheckFitterAndInvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutFitterInterface::FitType UiLayoutFitterComponent::GetFitType()
{
    if (m_horizontalFit && m_verticalFit)
    {
        return UiLayoutFitterInterface::FitType::HorizontalAndVertical;
    }
    else if (m_horizontalFit)
    {
        return UiLayoutFitterInterface::FitType::HorizontalOnly;
    }
    else if (m_verticalFit)
    {
        return UiLayoutFitterInterface::FitType::VerticalOnly;
    }
    else
    {
        return UiLayoutFitterInterface::FitType::None;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiLayoutFitterComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiLayoutFitterComponent, AZ::Component>()
            ->Version(0)
            ->Field("HorizontalFit", &UiLayoutFitterComponent::m_horizontalFit)
            ->Field("VerticalFit", &UiLayoutFitterComponent::m_verticalFit);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiLayoutFitterComponent>("LayoutFitter", "A component that resizes its element to its content.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiLayoutFitter.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiLayoutFitter.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutFitterComponent::m_horizontalFit, "Horizontal Fit",
                "When checked, this element will be resized according to the target width of its content.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutFitterComponent::CheckFitterAndInvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutFitterComponent::RefreshEditorTransformProperties);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutFitterComponent::m_verticalFit, "Vertical Fit",
                "When checked, this element will be resized according to the target height of its content.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutFitterComponent::CheckFitterAndInvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutFitterComponent::RefreshEditorTransformProperties);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiLayoutFitterBus>("UiLayoutFitterBus")
            ->Event("GetHorizontalFit", &UiLayoutFitterBus::Events::GetHorizontalFit)
            ->Event("SetHorizontalFit", &UiLayoutFitterBus::Events::SetHorizontalFit)
            ->Event("GetVerticalFit", &UiLayoutFitterBus::Events::GetVerticalFit)
            ->Event("SetVerticalFit", &UiLayoutFitterBus::Events::SetVerticalFit);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::Activate()
{
    UiLayoutControllerBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutFitterBus::Handler::BusConnect(m_entity->GetId());

    // If this is the first time the entity has been activated this has no effect since the canvas
    // is not known. But if a LayoutFitter component has just been pasted onto an existing entity
    // we need to invalidate the layout in case that affects things.
    CheckFitterAndInvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::Deactivate()
{
    UiLayoutControllerBus::Handler::BusDisconnect();
    UiLayoutFitterBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int UiLayoutFitterComponent::GetPriority() const
{
    // Priority should be lower (called earlier) than components that modify their children size
    // Default prioritiy is 100
    return 50;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::CheckFitterAndInvalidateLayout()
{
    if (m_horizontalFit || m_verticalFit)
    {
        UiLayoutHelpers::InvalidateLayout(GetEntityId());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutFitterComponent::RefreshEditorTransformProperties()
{
    UiEditorChangeNotificationBus::Broadcast(&UiEditorChangeNotificationBus::Events::OnEditorTransformPropertiesNeedRefresh);
}
