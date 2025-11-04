/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiLayoutCellComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutCellComponent::UiLayoutCellComponent()
    : m_minWidthOverridden(false)
    , m_minWidth(0.0f)
    , m_minHeightOverridden(false)
    , m_minHeight(0.0f)
    , m_targetWidthOverridden(false)
    , m_targetWidth(0.0f)
    , m_targetHeightOverridden(false)
    , m_targetHeight(0.0f)
    , m_maxWidthOverridden(false)
    , m_maxWidth(0.0f)
    , m_maxHeightOverridden(false)
    , m_maxHeight(0.0f)
    , m_extraWidthRatioOverridden(false)
    , m_extraWidthRatio(0.0f)
    , m_extraHeightRatioOverridden(false)
    , m_extraHeightRatio(0.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutCellComponent::~UiLayoutCellComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetMinWidth()
{
    return m_minWidthOverridden ? m_minWidth : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetMinWidth(float width)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(width))
    {
        m_minWidth = width;
        m_minWidthOverridden = true;
    }
    else
    {
        m_minWidth = LyShine::UiLayoutCellUnspecifiedSize;
        m_minWidthOverridden = false;
    }
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetMinHeight()
{
    return m_minHeightOverridden ? m_minHeight : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetMinHeight(float height)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(height))
    {
        m_minHeight = height;
        m_minHeightOverridden = true;
    }
    else
    {
        m_minHeight = LyShine::UiLayoutCellUnspecifiedSize;
        m_minHeightOverridden = false;
    }
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetTargetWidth()
{
    return m_targetWidthOverridden ? m_targetWidth : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetTargetWidth(float width)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(width))
    {
        m_targetWidth = width;
        m_targetWidthOverridden = true;
    }
    else
    {
        m_targetWidth = LyShine::UiLayoutCellUnspecifiedSize;
        m_targetWidthOverridden = false;
    }

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetTargetHeight()
{
    return m_targetHeightOverridden ? m_targetHeight : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetTargetHeight(float height)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(height))
    {
        m_targetHeight = height;
        m_targetHeightOverridden = true;
    }
    else
    {
        m_targetHeight = LyShine::UiLayoutCellUnspecifiedSize;
        m_targetHeightOverridden = false;
    }

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetMaxWidth()
{
    return m_maxWidthOverridden ? m_maxWidth : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetMaxWidth(float width)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(width))
    {
        m_maxWidth = width;
        m_maxWidthOverridden = true;
    }
    else
    {
        m_maxWidth = LyShine::UiLayoutCellUnspecifiedSize;
        m_maxWidthOverridden = false;
    }

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetMaxHeight()
{
    return m_maxHeightOverridden ? m_maxHeight : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetMaxHeight(float height)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(height))
    {
        m_maxHeight = height;
        m_maxHeightOverridden = true;
    }
    else
    {
        m_maxHeight = LyShine::UiLayoutCellUnspecifiedSize;
        m_maxHeightOverridden = false;
    }

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetExtraWidthRatio()
{
    return m_extraWidthRatioOverridden ? m_extraWidthRatio : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetExtraWidthRatio(float width)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(width))
    {
        m_extraWidthRatio = width;
        m_extraWidthRatioOverridden = true;
    }
    else
    {
        m_extraWidthRatio = LyShine::UiLayoutCellUnspecifiedSize;
        m_extraWidthRatioOverridden = false;
    }

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetExtraHeightRatio()
{
    return m_extraHeightRatioOverridden ? m_extraHeightRatio : LyShine::UiLayoutCellUnspecifiedSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetExtraHeightRatio(float height)
{
    if (LyShine::IsUiLayoutCellSizeSpecified(height))
    {
        m_extraHeightRatio = height;
        m_extraHeightRatioOverridden = true;
    }
    else
    {
        m_extraHeightRatio = LyShine::UiLayoutCellUnspecifiedSize;
        m_extraHeightRatioOverridden = false;
    }

    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiLayoutCellComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiLayoutCellComponent, AZ::Component>()
            ->Version(1)
            ->Field("MinWidthOverridden", &UiLayoutCellComponent::m_minWidthOverridden)
            ->Field("MinWidth", &UiLayoutCellComponent::m_minWidth)
            ->Field("MinHeightOverridden", &UiLayoutCellComponent::m_minHeightOverridden)
            ->Field("MinHeight", &UiLayoutCellComponent::m_minHeight)
            ->Field("TargetWidthOverridden", &UiLayoutCellComponent::m_targetWidthOverridden)
            ->Field("TargetWidth", &UiLayoutCellComponent::m_targetWidth)
            ->Field("TargetHeightOverridden", &UiLayoutCellComponent::m_targetHeightOverridden)
            ->Field("TargetHeight", &UiLayoutCellComponent::m_targetHeight)
            ->Field("MaxWidthOverridden", &UiLayoutCellComponent::m_maxWidthOverridden)
            ->Field("MaxWidth", &UiLayoutCellComponent::m_maxWidth)
            ->Field("MaxHeightOverridden", &UiLayoutCellComponent::m_maxHeightOverridden)
            ->Field("MaxHeight", &UiLayoutCellComponent::m_maxHeight)
            ->Field("ExtraWidthRatioOverridden", &UiLayoutCellComponent::m_extraWidthRatioOverridden)
            ->Field("ExtraWidthRatio", &UiLayoutCellComponent::m_extraWidthRatio)
            ->Field("ExtraHeightRatioOverridden", &UiLayoutCellComponent::m_extraHeightRatioOverridden)
            ->Field("ExtraHeightRatio", &UiLayoutCellComponent::m_extraHeightRatio);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiLayoutCellComponent>("LayoutCell", "Allows default layout cell properties to be overridden.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiLayoutCell.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiLayoutCell.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_minWidthOverridden, "Min Width",
                "Check this box to override the minimum width.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_minWidth, "Value",
                "Specify minimum width.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_minWidthOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_minHeightOverridden, "Min Height",
                "Check this box to override the minimum height.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_minHeight, "Value",
                "Specify minimum height.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_minHeightOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_targetWidthOverridden, "Target Width",
                "Check this box to override the target width.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_targetWidth, "Value",
                "Specify target width.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_targetWidthOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_targetHeightOverridden, "Target Height",
                "Check this box to override the target height.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_targetHeight, "Value",
                "Specify target height.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_targetHeightOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_maxWidthOverridden, "Max Width",
                "Check this box to override the max width.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_maxWidth, "Value",
                "Specify max width.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_maxWidthOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_maxHeightOverridden, "Max Height",
                "Check this box to override the max height.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_maxHeight, "Value",
                "Specify max height.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_maxHeightOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_extraWidthRatioOverridden, "Extra Width Ratio",
                "Check this box to override the extra width ratio.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_extraWidthRatio, "Value",
                "Specify extra width ratio.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_extraWidthRatioOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_extraHeightRatioOverridden, "Extra Height Ratio",
                "Check this box to override the extra height ratio.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_extraHeightRatio, "Value",
                "Specify extra height ratio.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_extraHeightRatioOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateLayout);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiLayoutCellBus>("UiLayoutCellBus")
            ->Event("GetMinWidth", &UiLayoutCellBus::Events::GetMinWidth)
            ->Event("SetMinWidth", &UiLayoutCellBus::Events::SetMinWidth)
            ->Event("GetMinHeight", &UiLayoutCellBus::Events::GetMinHeight)
            ->Event("SetMinHeight", &UiLayoutCellBus::Events::SetMinHeight)
            ->Event("GetTargetWidth", &UiLayoutCellBus::Events::GetTargetWidth)
            ->Event("SetTargetWidth", &UiLayoutCellBus::Events::SetTargetWidth)
            ->Event("GetTargetHeight", &UiLayoutCellBus::Events::GetTargetHeight)
            ->Event("SetTargetHeight", &UiLayoutCellBus::Events::SetTargetHeight)
            ->Event("GetMaxWidth", &UiLayoutCellBus::Events::GetMaxWidth)
            ->Event("SetMaxWidth", &UiLayoutCellBus::Events::SetMaxWidth)
            ->Event("GetMaxHeight", &UiLayoutCellBus::Events::GetMaxHeight)
            ->Event("SetMaxHeight", &UiLayoutCellBus::Events::SetMaxHeight)
            ->Event("GetExtraWidthRatio", &UiLayoutCellBus::Events::GetExtraWidthRatio)
            ->Event("SetExtraWidthRatio", &UiLayoutCellBus::Events::SetExtraWidthRatio)
            ->Event("GetExtraHeightRatio", &UiLayoutCellBus::Events::GetExtraHeightRatio)
            ->Event("SetExtraHeightRatio", &UiLayoutCellBus::Events::SetExtraHeightRatio)
            ->VirtualProperty("MinWidth", "GetMinWidth", "SetMinWidth")
            ->VirtualProperty("MinHeight", "GetMinHeight", "SetMinHeight")
            ->VirtualProperty("TargetWidth", "GetTargetWidth", "SetTargetWidth")
            ->VirtualProperty("TargetHeight", "GetTargetHeight", "SetTargetHeight")
            ->VirtualProperty("MaxWidth", "GetMaxWidth", "SetMaxWidth")
            ->VirtualProperty("MaxHeight", "GetMaxHeight", "SetMaxHeight")
            ->VirtualProperty("ExtraWidthRatio", "GetExtraWidthRatio", "SetExtraWidthRatio")
            ->VirtualProperty("ExtraHeightRatio", "GetExtraHeightRatio", "SetExtraHeightRatio");

        behaviorContext->Constant("UiLayoutCellUnspecifiedSize", BehaviorConstant(LyShine::UiLayoutCellUnspecifiedSize));

        behaviorContext->Class<UiLayoutCellComponent>()->RequestBus("UiLayoutCellBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::Activate()
{
    UiLayoutCellBus::Handler::BusConnect(GetEntityId());

    // If this is the first time the entity has been activated this has no effect since the canvas
    // is not known. But if a LayoutCell component has just been pasted onto an existing entity
    // we need to invalidate the layout in case that affects things.
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::Deactivate()
{
    UiLayoutCellBus::Handler::BusDisconnect();

    // We could be about to remove this component and then reactivate the entity
    // which could affect the layout if there is a parent layout component
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::InvalidateLayout()
{
    // Invalidate the parent's layout
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    UiLayoutManagerBus::Event(
        canvasEntityId, &UiLayoutManagerBus::Events::MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), false);

    // Invalidate the element's layout
    UiLayoutManagerBus::Event(canvasEntityId, &UiLayoutManagerBus::Events::MarkToRecomputeLayout, GetEntityId());
}
