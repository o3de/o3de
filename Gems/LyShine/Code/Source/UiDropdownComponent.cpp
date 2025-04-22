/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiDropdownComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiDropdownOptionBus.h>
#include <LyShine/UiSerializeHelpers.h>
#include "UiNavigationHelpers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDropdownNotificationBusBehaviorHandler  Behavior context handler class
class UiDropdownNotificationBusBehaviorHandler
    : public UiDropdownNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiDropdownNotificationBusBehaviorHandler, "{C936F190-524E-410E-82C9-9B590015B6D5}", AZ::SystemAllocator,
        OnDropdownExpanded, OnDropdownCollapsed, OnDropdownValueChanged);

    void OnDropdownExpanded() override
    {
        Call(FN_OnDropdownExpanded);
    }

    void OnDropdownCollapsed() override
    {
        Call(FN_OnDropdownCollapsed);
    }

    void OnDropdownValueChanged(AZ::EntityId value) override
    {
        Call(FN_OnDropdownValueChanged, value);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropdownComponent::UiDropdownComponent()
    : m_value()
    , m_content()
    , m_expandOnHover(false)
    , m_waitTime(0.3f)
    , m_collapseOnOutsideClick(true)
    , m_expandedParentId()
    , m_textElement()
    , m_iconElement()
    , m_expandedActionName()
    , m_collapsedActionName()
    , m_optionSelectedActionName()
    , m_expanded(false)
    , m_canvasEntityId()
    , m_delayTimer(0.f)
    , m_baseParent()
    , m_expandedByClick(true)
{
    m_stateActionManager.AddState(&m_expandedStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropdownComponent::~UiDropdownComponent()
{
    // delete all the state actions now rather than letting the base class do it automatically
    // because the m_stateActionManager has pointers to members in this derived class.
    m_stateActionManager.ClearStates();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownComponent::GetValue()
{
    return m_value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetValue(AZ::EntityId value)
{
    m_value = value;

    // Get the text from the newly selection option
    AZ::EntityId optionText;
    UiDropdownOptionBus::EventResult(optionText, value, &UiDropdownOptionBus::Events::GetTextElement);
    if (optionText.IsValid())
    {
        AZStd::string text;
        UiTextBus::EventResult(text, optionText, &UiTextBus::Events::GetText);
        // Set our text to that text to show which option was selected
        UiTextBus::Event(m_textElement, &UiTextBus::Events::SetTextWithFlags, text, UiTextInterface::SetTextFlags::SetLocalized);
    }

    // Get the icon from the newly selection option
    AZ::EntityId optionIcon;
    UiDropdownOptionBus::EventResult(optionIcon, value, &UiDropdownOptionBus::Events::GetIconElement);
    if (optionIcon.IsValid())
    {
        ISprite* sprite;
        UiImageBus::EventResult(sprite, optionIcon, &UiImageBus::Events::GetSprite);
        // Set our icon to that icon to show which option was selected
        UiImageBus::Event(m_iconElement, &UiImageBus::Events::SetSprite, sprite);
    }

    if (!m_optionSelectedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(
            canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_optionSelectedActionName);
    }
    UiDropdownNotificationBus::Event(GetEntityId(), &UiDropdownNotificationBus::Events::OnDropdownValueChanged, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownComponent::GetContent()
{
    return m_content;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetContent(AZ::EntityId content)
{
    m_content = content;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::GetExpandOnHover()
{
    return m_expandOnHover;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetExpandOnHover(bool expandOnHover)
{
    m_expandOnHover = expandOnHover;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDropdownComponent::GetWaitTime()
{
    return m_waitTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetWaitTime(float waitTime)
{
    m_waitTime = waitTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::GetCollapseOnOutsideClick()
{
    return m_collapseOnOutsideClick;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetCollapseOnOutsideClick(bool collapseOnOutsideClick)
{
    m_collapseOnOutsideClick = collapseOnOutsideClick;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownComponent::GetExpandedParentId()
{
    return m_expandedParentId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetExpandedParentId(AZ::EntityId expandedParentId)
{
    m_expandedParentId = expandedParentId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownComponent::GetTextElement()
{
    return m_textElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetTextElement(AZ::EntityId textElement)
{
    m_textElement = textElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownComponent::GetIconElement()
{
    return m_iconElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetIconElement(AZ::EntityId iconElement)
{
    m_iconElement = iconElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::Expand()
{
    Expand(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::Collapse()
{
    Collapse(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiDropdownComponent::GetExpandedActionName()
{
    return m_expandedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetExpandedActionName(const LyShine::ActionName& actionName)
{
    m_expandedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiDropdownComponent::GetCollapsedActionName()
{
    return m_collapsedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetCollapsedActionName(const LyShine::ActionName& actionName)
{
    m_collapsedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiDropdownComponent::GetOptionSelectedActionName()
{
    return m_optionSelectedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::SetOptionSelectedActionName(const LyShine::ActionName& actionName)
{
    m_optionSelectedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::InGamePostActivate()
{
    // If the dropdown content is an interactable set its navigation to none
    UiNavigationBus::Event(m_content, &UiNavigationBus::Events::SetNavigationMode, UiNavigationInterface::NavigationMode::None);

    // Hide the dropdown on game start
    UiElementBus::Event(m_content, &UiElementBus::Events::SetIsEnabled, false);

    // Connect to canvas input notifications
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    UiCanvasInputNotificationBus::Handler::BusConnect(canvasEntityId);
    m_canvasEntityId = canvasEntityId;

    // Save the base parent for the content
    UiElementBus::EventResult(m_baseParent, m_content, &UiElementBus::Events::GetParentEntityId);

    // Get a list of all our submenus (content descendants that have a dropdown component)
    UiElementBus::Event(
        m_content,
        &UiElementBus::Events::FindDescendantElements,
        [](const AZ::Entity* entity)
        {
            return UiDropdownBus::FindFirstHandler(entity->GetId()) != nullptr;
        },
        m_submenus);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::HandleReleased(AZ::Vector2 point)
{
    bool isInRect = false;
    UiTransformBus::EventResult(isInRect, GetEntityId(), &UiTransformBus::Events::IsPointInRect, point);
    if (isInRect)
    {
        return HandleReleasedCommon(point);
    }
    else
    {
        m_isPressed = false;

        return m_isHandlingEvents;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::HandleEnterReleased()
{
    AZ::Vector2 point(-1.0f, -1.0f);
    return HandleReleasedCommon(point);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::HandleHoverStart()
{
    m_isHover = true;

    UiInteractableComponent::TriggerHoverStartAction();

    if (m_expandOnHover && !m_expanded)
    {
        // Reset the timer and start listening to tick events to expand the menu
        m_delayTimer = 0.f;
        AZ::TickBus::Handler::BusConnect();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::HandleHoverEnd()
{
    m_isHover = false;

    UiInteractableComponent::TriggerHoverEndAction();

    if (m_expandOnHover)
    {
        if (m_expanded && !m_expandedByClick)
        {
            // Reset the timer and start listening to tick events to collapse the menu
            m_delayTimer = 0.f;
            AZ::TickBus::Handler::BusConnect();
        }
        else if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::OnReceivedHoverByNavigatingFromDescendant([[maybe_unused]] AZ::EntityId descendantEntityId)
{
    AZ::EntityId entityId = *UiInteractableNotificationBus::GetCurrentBusId();

    if (entityId == m_tempContentParentInteractable)
    {
        Collapse(true);

        // Disconnect from the tickbus if we were connected
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::OnCanvasPrimaryReleased(AZ::EntityId entityId)
{
    HandleCanvasReleasedCommon(entityId, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::OnCanvasEnterReleased(AZ::EntityId entityId)
{
    if (entityId.IsValid())
    {
        HandleCanvasReleasedCommon(entityId, false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::OnCanvasHoverStart(AZ::EntityId entityId)
{
    if (entityId == m_tempContentParentInteractable)
    {
        TransferHoverToDescendant();
    }
    else
    {
        // We only care about hovered things when we're already expanded
        if (m_expandOnHover && m_expanded)
        {
            // Figure out if the hovered entity is a descendant of either our content, or one of our
            // submenus content
            bool contentIsAncestor = ContentIsAncestor(entityId);

            // If we started hovering over one of our (or submenus) descendants or the dropdown button
            // and we were trying to collapse the menu
            if ((contentIsAncestor || entityId == GetEntityId()) && AZ::TickBus::Handler::BusIsConnected())
            {
                // Stop trying to collapse the menu
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::OnCanvasHoverEnd(AZ::EntityId entityId)
{
    // We only care about hovered things when we're already expanded
    if (m_expandOnHover && m_expanded && !m_expandedByClick)
    {
        // Figure out if the hovered entity is a descendant of either our content, or one of our
        // submenus content
        bool contentIsAncestor = ContentIsAncestor(entityId);

        // If we stopped hovering over one of our (or submenus) descendants
        if (contentIsAncestor)
        {
            // Reset the timer and start listening to tick events
            m_delayTimer = 0.f;
            AZ::TickBus::Handler::BusConnect();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
{
    m_delayTimer += deltaTime;

    // If we went over the wait time
    if (m_delayTimer >= m_waitTime)
    {
        // If we were waiting to expand
        if (!m_expanded)
        {
            m_expandedByClick = false;
            Expand();
        }
        // Else we were waiting to collapse
        else
        {
            Collapse();
        }
        // (we won't listen to the tick bus if we are not in either case)
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiDropdownBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiDropdownBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());
    if (m_canvasEntityId.IsValid())
    {
        UiCanvasInputNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
    }
    if (AZ::TickBus::Handler::BusIsConnected())
    {
        AZ::TickBus::Handler::BusDisconnect();
    }
    if (m_tempContentParentInteractable.IsValid())
    {
        UiInteractableNotificationBus::MultiHandler::BusDisconnect(m_tempContentParentInteractable);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiDropdownComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiDropdownComponent, UiInteractableComponent>()
            ->Version(1)
        // Elements group
            ->Field("Content", &UiDropdownComponent::m_content)
            ->Field("ExpandedParent", &UiDropdownComponent::m_expandedParentId)
            ->Field("TextElement", &UiDropdownComponent::m_textElement)
            ->Field("IconElement", &UiDropdownComponent::m_iconElement)
        // Options group
            ->Field("ExpandOnHover", &UiDropdownComponent::m_expandOnHover)
            ->Field("WaitTime", &UiDropdownComponent::m_waitTime)
            ->Field("CollapseOnOutsideClick", &UiDropdownComponent::m_collapseOnOutsideClick)
        // Dropdown States group
            ->Field("ExpandedStateActions", &UiDropdownComponent::m_expandedStateActions)
        // Actions group
            ->Field("ExpandedActionName", &UiDropdownComponent::m_expandedActionName)
            ->Field("CollapsedActionName", &UiDropdownComponent::m_collapsedActionName)
            ->Field("OptionSelectedActionName", &UiDropdownComponent::m_optionSelectedActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiDropdownComponent>("Dropdown", "An interactable component for Dropdown behavior.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiDropdown.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiDropdown.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Elements group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Elements")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiDropdownComponent::m_content, "Content", "The element that contains the dropdown list.")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &UiDropdownComponent::ValidatePotentialContent)
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiDropdownComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::EntityId, &UiDropdownComponent::m_expandedParentId, "Expanded Parent", "The element the dropdown content should parent to when expanded (the canvas by default)."
                    "This is used for layering, to display the dropdown content over other elements in the canvas that might be after it in the hierarchy.")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &UiDropdownComponent::ValidatePotentialExpandedParent);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiDropdownComponent::m_textElement, "Text Element", "The text element to use to display which option is selected.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiDropdownComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiDropdownComponent::m_iconElement, "Icon Element", "The icon element to use to display which option is selected.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiDropdownComponent::PopulateChildEntityList);
            }

            // Options group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Options")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiDropdownComponent::m_expandOnHover, "Expand on Hover", "Whether this dropdown should be expanded upon hover, and collapse upon exit.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

                editInfo->DataElement(0, &UiDropdownComponent::m_waitTime, "Wait Time", "How long the dropdown should wait before expanding on hover or collapsing on exit.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiDropdownComponent::GetExpandOnHover);

                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiDropdownComponent::m_collapseOnOutsideClick, "Collapse on Outside Click", "Whether this dropdown should be collapsed upon clicking outside the menu.");
            }

            // Dropdown States group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Dropdown States")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiDropdownComponent::m_expandedStateActions, "Expanded", "The expanded state actions.")
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &UiDropdownComponent::OnExpandedStateActionsChanged);
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiDropdownComponent::m_expandedActionName, "Expanded", "The action triggered when the dropdown is expanded.");
                editInfo->DataElement(0, &UiDropdownComponent::m_collapsedActionName, "Collapsed", "The action triggered when the dropdown is collapsed.");
                editInfo->DataElement(0, &UiDropdownComponent::m_optionSelectedActionName, "Option Selected", "The action triggered when an option is selected.");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiDropdownBus>("UiDropdownBus")
            ->Event("GetValue", &UiDropdownBus::Events::GetValue)
            ->Event("SetValue", &UiDropdownBus::Events::SetValue)
            ->Event("GetContent", &UiDropdownBus::Events::GetContent)
            ->Event("SetContent", &UiDropdownBus::Events::SetContent)
            ->Event("GetExpandOnHover", &UiDropdownBus::Events::GetExpandOnHover)
            ->Event("SetExpandOnHover", &UiDropdownBus::Events::SetExpandOnHover)
            ->Event("GetWaitTime", &UiDropdownBus::Events::GetWaitTime)
            ->Event("SetWaitTime", &UiDropdownBus::Events::SetWaitTime)
            ->Event("GetCollapseOnOutsideClick", &UiDropdownBus::Events::GetCollapseOnOutsideClick)
            ->Event("SetCollapseOnOutsideClick", &UiDropdownBus::Events::SetCollapseOnOutsideClick)
            ->Event("GetExpandedParentId", &UiDropdownBus::Events::GetExpandedParentId)
            ->Event("SetExpandedParentId", &UiDropdownBus::Events::SetExpandedParentId)
            ->Event("GetTextElement", &UiDropdownBus::Events::GetTextElement)
            ->Event("SetTextElement", &UiDropdownBus::Events::SetTextElement)
            ->Event("GetIconElement", &UiDropdownBus::Events::GetIconElement)
            ->Event("SetIconElement", &UiDropdownBus::Events::SetIconElement)
            ->Event("Expand", &UiDropdownBus::Events::Expand)
            ->Event("Collapse", &UiDropdownBus::Events::Collapse)
            ->Event("GetExpandedActionName", &UiDropdownBus::Events::GetExpandedActionName)
            ->Event("SetExpandedActionName", &UiDropdownBus::Events::SetExpandedActionName)
            ->Event("GetCollapsedActionName", &UiDropdownBus::Events::GetCollapsedActionName)
            ->Event("SetCollapsedActionName", &UiDropdownBus::Events::SetCollapsedActionName)
            ->Event("GetOptionSelectedActionName", &UiDropdownBus::Events::GetOptionSelectedActionName)
            ->Event("SetOptionSelectedActionName", &UiDropdownBus::Events::SetOptionSelectedActionName);

        behaviorContext->EBus<UiDropdownNotificationBus>("UiDropdownNotificationBus")
            ->Handler<UiDropdownNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropdownComponent::EntityComboBoxVec UiDropdownComponent::PopulateChildEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all child elements
    LyShine::EntityArray children;
    UiElementBus::EventResult(children, GetEntityId(), &UiElementBus::Events::GetChildElements);

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : children)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::OnExpandedStateActionsChanged()
{
    m_stateActionManager.InitInteractableEntityForStateActions(m_expandedStateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::Expand(bool transferHover)
{
    m_expanded = true;

    // Enable the dropdown menu
    UiElementBus::Event(m_content, &UiElementBus::Events::SetIsEnabled, true);

    // Disconnect from the tickbus if we were connected
    if (AZ::TickBus::Handler::BusIsConnected())
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    // Save the current viewport position and scale
    AZ::Vector2 viewportPosition;
    UiTransformBus::EventResult(viewportPosition, m_content, &UiTransformBus::Events::GetViewportPosition);

    // Create a temporary content parent interactable that's a child of the given expanded parent
    // or the canvas if no expanded parent was specified.
    // The content element needs a parent interactable to constrain navigation between the content's
    // descendant interactables.
    m_tempContentParentInteractable = CreateContentParentInteractable();

    // Reparent the dropdown content to the content parent interactable
    if (m_tempContentParentInteractable.IsValid())
    {
        UiInteractableNotificationBus::MultiHandler::BusConnect(m_tempContentParentInteractable);
        UiElementBus::Event(m_content, &UiElementBus::Events::ReparentByEntityId, m_tempContentParentInteractable, AZ::EntityId());
    }

    UiTransformBus::Event(m_content, &UiTransformBus::Events::SetViewportPosition, viewportPosition);

    if (transferHover && IsNavigationSupported())
    {
        // Set the first descendant interactable to have the hover
        TransferHoverToDescendant();
    }

    if (!m_expandedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_expandedActionName);
    }
    UiDropdownNotificationBus::Event(GetEntityId(), &UiDropdownNotificationBus::Events::OnDropdownExpanded);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::Collapse(bool transferHover)
{
    bool curHoverInteractableIsAncestor = false;

    AZ::EntityId hoverInteractable;
    UiCanvasBus::EventResult(hoverInteractable, m_canvasEntityId, &UiCanvasBus::Events::GetHoverInteractable);
    if (hoverInteractable.IsValid() && hoverInteractable != GetEntityId())
    {
        if (ContentIsAncestor(hoverInteractable))
        {
            curHoverInteractableIsAncestor = true;
        }
    }

    if (IsNavigationSupported() && curHoverInteractableIsAncestor)
    {
        if (transferHover)
        {
            // Regain the hover
            UiCanvasBus::Event(m_canvasEntityId, &UiCanvasBus::Events::ForceHoverInteractable, GetEntityId());
        }
        else
        {
            // Make sure a soon to be disabled interactable doesn't remain the hover interactable
            UiCanvasBus::Event(m_canvasEntityId, &UiCanvasBus::Events::ForceHoverInteractable, AZ::EntityId());
        }
    }

    m_expanded = false;

    // This is for Expand to always work the same way when called by script
    m_expandedByClick = true;

    // Disable the dropdown menu
    UiElementBus::Event(m_content, &UiElementBus::Events::SetIsEnabled, false);

    // Disconnect from the tickbus if we were connected
    if (AZ::TickBus::Handler::BusIsConnected())
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    // Save the current viewport position and scale
    AZ::Vector2 viewportPosition;
    UiTransformBus::EventResult(viewportPosition, m_content, &UiTransformBus::Events::GetViewportPosition);

    // Reparent the dropdown content to the base collapsed parent
    if (m_baseParent.IsValid())
    {
        UiElementBus::Event(m_content, &UiElementBus::Events::ReparentByEntityId, m_baseParent, AZ::EntityId());
    }
    // If the dropdown content had no base collapsed parent, reparent to canvas
    else
    {
        UiElementBus::Event(m_content, &UiElementBus::Events::Reparent, nullptr, nullptr);
    }

    // Destroy the temporary content parent interactable
    if (m_tempContentParentInteractable.IsValid())
    {
        UiInteractableNotificationBus::MultiHandler::BusDisconnect(m_tempContentParentInteractable);
        UiElementBus::Event(m_tempContentParentInteractable, &UiElementBus::Events::DestroyElement);
        m_tempContentParentInteractable.SetInvalid();
    }

    UiTransformBus::Event(m_content, &UiTransformBus::Events::SetViewportPosition, viewportPosition);

    if (!m_collapsedActionName.empty())
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_collapsedActionName);
    }
    UiDropdownNotificationBus::Event(GetEntityId(), &UiDropdownNotificationBus::Events::OnDropdownCollapsed);

    // Let all our submenus know they should collapse
    for (auto submenu : m_submenus)
    {
        UiDropdownBus::Event(submenu->GetId(), &UiDropdownBus::Events::Collapse);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Outcome<void, AZStd::string> UiDropdownComponent::ValidateTypeIsEntityId(const AZ::Uuid& valueType)
{
    if (azrtti_typeid<AZ::EntityId>() != valueType)
    {
        AZ_Assert(false, "Unexpected value type");
        return AZ::Failure(AZStd::string("Trying to set an entity ID to something that isn't an entity ID!"));
    }
    return AZ::Success();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Outcome<void, AZStd::string> UiDropdownComponent::ValidatePotentialContent(void* newValue, const AZ::Uuid& valueType)
{
    auto typeValidation = ValidateTypeIsEntityId(valueType);

    if (!typeValidation.IsSuccess()) {
        return typeValidation;
    }

    AZ::EntityId actualValue = *static_cast<AZ::EntityId*>(newValue);

    // Don't allow the change if it will result in a cycle hierarchy
    if (actualValue.IsValid() && actualValue == m_expandedParentId)
    {
        return AZ::Failure(AZStd::string("You cannot set content to be the same as expanded parent!"));
    }

    if (ContentIsAncestor(m_expandedParentId, actualValue))
    {
        return AZ::Failure(AZStd::string("You cannot set content to be an ancestor of expanded parent!"));
    }

    return AZ::Success();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Outcome<void, AZStd::string> UiDropdownComponent::ValidatePotentialExpandedParent(void* newValue, const AZ::Uuid& valueType)
{
    auto typeValidation = ValidateTypeIsEntityId(valueType);

    if (!typeValidation.IsSuccess()) {
        return typeValidation;
    }

    AZ::EntityId actualValue = *static_cast<AZ::EntityId*>(newValue);

    // Don't allow the change if it will result in a cycle hierarchy
    if (actualValue.IsValid() && actualValue == m_content)
    {
        return AZ::Failure(AZStd::string("You cannot set expanded parent to be the same as content!"));
    }

    if (ContentIsAncestor(actualValue))
    {
        return AZ::Failure(AZStd::string("You cannot set expanded parent to be a child of content!"));
    }

    return AZ::Success();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::HandleReleasedCommon(const AZ::Vector2& point)
{
    if (m_isHandlingEvents)
    {
        UiInteractableComponent::TriggerReleasedAction();

        bool transferHover = (point == AZ::Vector2(-1.0f, -1.0f));

        if (!m_expanded)
        {
            if (m_expandOnHover)
            {
                m_expandedByClick = true;
            }
            Expand(transferHover);
        }
        else
        {
            // Only collapse if it's not an expand on hover dropdown or if it was expanded
            // by a click if it is an expand on hover dropdown
            if (!m_expandOnHover || m_expandedByClick)
            {
                Collapse(transferHover);
            }
        }
    }

    m_isPressed = false;

    return m_isHandlingEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::HandleCanvasReleasedCommon(AZ::EntityId entityId, bool positionalInput)
{
    if (m_expanded)
    {
        // Collapse the menu in the following cases:
        // - the user clicked on the dropdown button
        // - the user clicked on an option
        // - the user clicked outside the dropdown

        // If the user clicked on the dropdown button
        if (entityId == GetEntityId())
        {
            // Let HandleReleasedCommon handle it
            return;
        }
        else
        {
            bool transferHover = !positionalInput;

            // Get the dropdown the option belongs to
            AZ::EntityId owningDropdown;
            UiDropdownOptionBus::EventResult(owningDropdown, entityId, &UiDropdownOptionBus::Events::GetOwningDropdown);
            // If one of our options was clicked
            if (owningDropdown == GetEntityId())
            {
                Collapse(transferHover);
                return;
            }
            else if (m_collapseOnOutsideClick)
            {
                if (entityId != m_content)
                {
                    // Figure out if the clicked entity is a descendant of either our content, or one of our
                    // submenus content
                    bool contentIsAncestor = ContentIsAncestor(entityId);
                    // If it was not an ancestor, then we clicked outside the dropdown
                    if (!contentIsAncestor)
                    {
                        Collapse(transferHover);
                        return;
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownComponent::TransferHoverToDescendant()
{
    // Find the first descendant interactable of the content element
    AZ::EntityId descendantInteractable = FindFirstDescendantInteractable(m_content);
    if (descendantInteractable.IsValid())
    {
        UiCanvasBus::Event(m_canvasEntityId, &UiCanvasBus::Events::ForceHoverInteractable, descendantInteractable);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownComponent::FindFirstDescendantInteractable(AZ::EntityId parentEntityId)
{
    AZ::EntityId firstDescendant;

    AZStd::vector<AZ::EntityId> childEntityIds;
    UiElementBus::EventResult(childEntityIds, parentEntityId, &UiElementBus::Events::GetChildEntityIds);

    for (auto childEntityId : childEntityIds)
    {
        if (UiNavigationHelpers::IsElementInteractableAndNavigable(childEntityId))
        {
            firstDescendant = childEntityId;
            break;
        }

        firstDescendant = FindFirstDescendantInteractable(childEntityId);
        if (firstDescendant.IsValid())
        {
            break;
        }
    }

    return firstDescendant;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownComponent::CreateContentParentInteractable()
{
    AZ::Entity* button = nullptr;
    if (m_expandedParentId.IsValid())
    {
        UiElementBus::EventResult(
            button, m_expandedParentId, &UiElementBus::Events::CreateChildElement, "InternalContentParentInteractable");
    }
    else
    {
        UiCanvasBus::EventResult(button, m_canvasEntityId, &UiCanvasBus::Events::CreateChildElement, "InternalContentParentInteractable");
    }

    AZ::EntityId buttonId;
    if (button)
    {
        // Set up the button element
        button->Deactivate();
        button->CreateComponent(LyShine::UiTransform2dComponentUuid);
        button->CreateComponent(LyShine::UiButtonComponentUuid);
        button->Activate();

        buttonId = button->GetId();

        AZ_Assert(UiTransform2dBus::FindFirstHandler(buttonId), "Transform2d component missing");

        UiTransform2dInterface::Anchors anchors(0.5f, 0.5f, 0.5f, 0.5f);
        UiTransform2dInterface::Offsets offsets(0.0f, 0.0f, 0.0f, 0.0f);
        AZ::Vector2 pivot(0.5f, 0.5f);
        UiTransform2dBus::Event(buttonId, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);
        UiTransform2dBus::Event(buttonId, &UiTransform2dBus::Events::SetOffsets, offsets);
        UiTransformBus::Event(buttonId, &UiTransformBus::Events::SetPivot, pivot);

        UiTransformInterface::RectPoints contentPoints;
        UiTransformBus::Event(m_content, &UiTransformBus::Events::GetViewportSpacePoints, contentPoints);
        UiTransformBus::Event(buttonId, &UiTransformBus::Events::SetViewportPosition, contentPoints.GetCenter());
    }

    return buttonId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::ContentIsAncestor(AZ::EntityId entityId)
{
    return ContentIsAncestor(entityId, m_content);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::ContentIsAncestor(AZ::EntityId entityId, AZ::EntityId contentId)
{
    bool contentIsAncestor = false;
    UiElementBus::EventResult(contentIsAncestor, entityId, &UiElementBus::Events::IsAncestor, contentId);
    if (contentIsAncestor)
    {
        return true;
    }

    for (auto submenu : m_submenus)
    {
        AZ::EntityId submenuContent;
        UiDropdownBus::EventResult(submenuContent, submenu->GetId(), &UiDropdownBus::Events::GetContent);
        bool submenuIsAncestor = false;
        UiElementBus::EventResult(submenuIsAncestor, entityId, &UiElementBus::Events::IsAncestor, submenuContent);
        if (submenuIsAncestor)
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDropdownComponent::IsNavigationSupported()
{
    bool isNavigationSupported = false;
    UiCanvasBus::EventResult(isNavigationSupported, m_canvasEntityId, &UiCanvasBus::Events::GetIsNavigationSupported);

    return isNavigationSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStatesInterface::State UiDropdownComponent::ComputeInteractableState()
{
    UiInteractableStatesInterface::State state = UiInteractableStatesInterface::StateNormal;

    if (!m_isHandlingEvents)
    {
        state = UiInteractableStatesInterface::StateDisabled;
    }
    else if (m_isPressed)
    {
        state = UiInteractableStatesInterface::StatePressed;
    }
    else if (m_isHover)
    {
        state = UiInteractableStatesInterface::StateHover;
    }
    else if (m_expanded)
    {
        state = DropdownStateExpanded;
    }

    return state;
}

