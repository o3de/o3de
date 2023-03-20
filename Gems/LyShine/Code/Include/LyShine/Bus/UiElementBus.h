/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <LyShine/UiBase.h>

namespace LyShine
{
    class IRenderGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiElementInterface
    : public AZ::ComponentBus
{
public: // member functions

    //! Deleting an element will remove it from its parent and delete its child elements and components
    virtual ~UiElementInterface() {}

    //! Render the element and its child elements and components, this is done by adding primitives to the render graph
    //! \param renderGraph, the render graph being added to
    //! \param isInGame, true if element being rendered in game (or preview), false if being render in edit mode
    virtual void RenderElement(LyShine::IRenderGraph* renderGraph, bool isInGame) = 0;

    //! Retrieves the identifier of this element.
    virtual LyShine::ElementId GetElementId() = 0;

    //! Get the name of this element
    virtual LyShine::NameType GetName() = 0;

    //! Get the canvas that contains this element (returns AZ::InvalidEntityId if element has no canvas)
    virtual AZ::EntityId GetCanvasEntityId() = 0;

    //! Get the parent element of this element (returns nullptr if element has no parent)
    virtual AZ::Entity* GetParent() = 0;

    //! Get the parent entity Id of this element (returns invalid Id if element has no parent)
    virtual AZ::EntityId GetParentEntityId() = 0;

    //! Get the number child elements of this element
    virtual int GetNumChildElements() = 0;

    //! Get the specified child element, index must be less than GetNumChildElements()
    virtual AZ::Entity* GetChildElement(int index) = 0;

    //! Get the specified child entity Id, index must be less than GetNumChildElements()
    virtual AZ::EntityId GetChildEntityId(int index) = 0;

    //! Get the specified child's UiElementInterface, index must be less than GetNumChildElements()
    //! and the element must be fully initialized
    virtual UiElementInterface* GetChildElementInterface(int index) = 0;

    //! Get the index of the specified child element
    virtual int GetIndexOfChild(const AZ::Entity* child) = 0;

    //! Get the index of the specified child element
    virtual int GetIndexOfChildByEntityId(AZ::EntityId childId) = 0;

    //! Get the child elements of this element
    virtual LyShine::EntityArray GetChildElements() = 0;

    //! Get the child entity Ids of this element
    virtual AZStd::vector<AZ::EntityId> GetChildEntityIds() = 0;

    //! Create a new element that is a child of this element, this element (the parent) has ownership of the child
    //! The new entity will have a UiElementComponent added but will not yet be initialized or activated
    virtual AZ::Entity* CreateChildElement(const LyShine::NameType& name) = 0;

    //! Destroy this element
    virtual void DestroyElement() = 0;

    //! Queue up element for destruction at end of frame
    virtual void DestroyElementOnFrameEnd() = 0;

    //! Re-parent this element to move it in the hierarchy
    //! \param newParent New parent element. If null then the canvas is the parent
    //! \param nextElement  Element to insert this element before. If null element is put at end of child list
    virtual void Reparent(AZ::Entity* newParent, AZ::Entity* insertBefore = nullptr) = 0;

    //! Re-parent this element to move it in the hierarchy
    //! \param newParent New parent element. If InvalidEntityId then the canvas is the parent
    //! \param nextElement  Element to insert this element before. If InvalidEntityId then element is put at end of child list
    virtual void ReparentByEntityId(AZ::EntityId newParent, AZ::EntityId insertBefore) = 0;

    //! Add this element as a child of the specified parent
    //! \param newParent New parent element. If null then the canvas is the parent
    //! \param index  Child index where element is inserted. If -1 element is put at end of child list
    virtual void AddToParentAtIndex(AZ::Entity* newParent, int index = -1) = 0;

    //! Remove this element from its parent
    virtual void RemoveFromParent() = 0;

    //! Get the front-most child element whose bounds include the given point in canvas space
    //! \return nullptr if no match
    virtual AZ::Entity* FindFrontmostChildContainingPoint(AZ::Vector2 point, bool isInGame) = 0;

    //! Get all the children whose bounds intersect with the given rect in canvas space
    //! \return Empty EntityArray if no match
    virtual LyShine::EntityArray FindAllChildrenIntersectingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1, bool isInGame) = 0;

    //! Look for an entity with interactable component to handle an event at given point
    virtual AZ::EntityId FindInteractableToHandleEvent(AZ::Vector2 point) = 0;

    //! Look for a parent (ancestor) entity with interactable component to handle dragging starting at given point
    virtual AZ::EntityId FindParentInteractableSupportingDrag(AZ::Vector2 point) = 0;

    //! Return the first immediate child element with the given name or nullptr if no match
    virtual AZ::Entity* FindChildByName(const LyShine::NameType& name) = 0;

    //! Return the first descendant element with the given name or nullptr if no match
    virtual AZ::Entity* FindDescendantByName(const LyShine::NameType& name) = 0;

    //! Return the first immediate child entity Id with the given name or invalid Id if no match
    virtual AZ::EntityId FindChildEntityIdByName(const LyShine::NameType& name) = 0;

    //! Return the first descendant entity Id with the given name or invalid Id if no match
    virtual AZ::EntityId FindDescendantEntityIdByName(const LyShine::NameType& name) = 0;

    //! Return the first immediate child element with the given id or nullptr if no match
    virtual AZ::Entity* FindChildByEntityId(AZ::EntityId id) = 0;

    //! Return the descendant element with the given id or nullptr if no match
    virtual AZ::Entity* FindDescendantById(LyShine::ElementId id) = 0;

    //! recursively find descendant elements matching a predicate
    //! \param result, any matching elements will be added to this array
    virtual void FindDescendantElements(AZStd::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result) = 0;

    //! recursively visit descendant elements and call the given function on them
    //! The function is called first on the element and then on its children
    virtual void CallOnDescendantElements(AZStd::function<void(const AZ::EntityId)> callFunction) = 0;

    //! Return whether a given element is an ancestor of this element
    virtual bool IsAncestor(AZ::EntityId id) = 0;

    //! Enabled/disabled
    virtual bool IsEnabled() = 0;
    virtual void SetIsEnabled(bool isEnabled) = 0;
    virtual bool GetAreElementAndAncestorsEnabled() = 0;

    //! This can be used to disable the render without disabling the update/interaction.
    //! This is used internally by components that temporarily disable rendering of other elements (though they preserve the existing value).
    virtual bool IsRenderEnabled() = 0;
    virtual void SetIsRenderEnabled(bool isRenderEnabled) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiElementInterface> UiElementBus;

// UI_ANIMATION_REVISIT This may be a temporary location

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiElementChangeNotification
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiElementChangeNotification() {}

    //! Notify listeners that a property has change on this entity
    virtual void UiElementPropertyChanged() {}
};

typedef AZ::EBus<UiElementChangeNotification> UiElementChangeNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiElementNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiElementNotifications() {}

    //! Notify listeners that the element is being destroyed
    virtual void OnUiElementBeingDestroyed() {}

    //! Notify listeners that the element has been fixed up (canvas and parent for the element have been set)
    virtual void OnUiElementFixup(AZ::EntityId /*canvasEntityId*/, AZ::EntityId /*parentEntityId*/) {}

    //! Notify listeners that the element has been enabled or disabled (the flag on this element was changed)
    virtual void OnUiElementEnabledChanged(bool /*isEnabled*/) {}

    //! Notify listeners that the element has been enabled or disabled either directly or to a change to an ancestors enabled flag
    virtual void OnUiElementAndAncestorsEnabledChanged(bool /*areElementAndAncestorsEnabled*/) {}
};

typedef AZ::EBus<UiElementNotifications> UiElementNotificationBus;
