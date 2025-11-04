/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiElementComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

#include "UiCanvasComponent.h"
#include <LyShine/IDraw2d.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiEditorBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiRenderControlBus.h>
#include <LyShine/Bus/UiInteractionMaskBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiEntityContextBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>

#include <CryCommon/StlUtils.h>

#include "UiTransform2dComponent.h"

#include "IConsole.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiElementComponent::UiElementComponent()
{
    // This is required in order to be able to tell if the element is in the scheduled transform
    // recompute list (intrusive_slist doesn't initialize this except in a debug build)
    m_next = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiElementComponent::~UiElementComponent()
{
    // If this element is currently in the list of elements needing a transform recompute then
    // remove it from that list since the element is being destroyed
    if (m_next)
    {
        if (m_canvas)
        {
            m_canvas->UnscheduleElementForTransformRecompute(this);
        }
        else
        {
            m_next = nullptr;
        }
    }

    // In normal (correct) usage we have nothing to do here.
    // But if a user calls DeleteEntity or just deletes an entity pointer they can delete a UI element
    // and leave its parent with a dangling child pointer.
    // So we report an error in that case and do some recovery code.

    // If we were being deleted via DestroyElement m_parentId would be invalid
    if (m_parentId.IsValid())
    {
        // Note we do not rely on the m_parent pointer because if the canvas is being unloaded for example the
        // parent entity could already have been deleted. So we use the parent entity Id to try to find the parent.
        AZ::Entity* parent = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(parent, &AZ::ComponentApplicationBus::Events::FindEntity, m_parentId);

        // If the parent is found and it is active that suggests something is wrong. When unloading a canvas we
        // deactivate all of the UI elements before any are deleted
        if (parent && parent->GetState() == AZ::Entity::State::Active)
        {
            // As a final check see if this element's parent thinks that this is a child, this is almost certain to be the
            // case if we got here but, if not, there is nothing more to do
            UiElementComponent* parentElementComponent = parent->FindComponent<UiElementComponent>();
            if (parentElementComponent)
            {
                if (parentElementComponent->FindChildByEntityId(GetEntityId()))
                {
                    // This is an error, report the error
                    AZ_Error("UI", false, "Deleting a UI element entity directly rather than using DestroyElement. Element is named '%s'", m_entity->GetName().c_str());

                    // Attempt to recover by removing this element from the parent's child list
                    parentElementComponent->RemoveChild(m_entity);

                    // And recursively delete any child UI elements (like DestroyElement on this element would have done)
                    auto childElementComponents = m_childElementComponents;
                    for (auto child : childElementComponents)
                    {
                        // destroy the child
                        child->DestroyElement();
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::RenderElement(LyShine::IRenderGraph* renderGraph, bool isInGame)
{
    if (!IsFullyInitialized())
    {
        return;
    }

    if (!m_isRenderEnabled)
    {
        return;
    }

    if (isInGame)
    {
        if (!m_isEnabled)
        {
            // Nothing to do - whole element and all children are disabled
            return;
        }
    }
    else
    {
        // We are in editing mode (not running the game)
        // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element is
        // hidden in the editor
        bool isVisible = true;
        UiEditorBus::EventResult(isVisible, GetEntityId(), &UiEditorBus::Events::GetIsVisible);
        if (!isVisible)
        {
            return;
        }
    }

    // If a component is connected to the UiRenderControl bus then we give control of rendering this element
    // and its children to that component, otherwise follow the standard render path
    if (m_renderControlInterface)
    {
        // give control of rendering this element and its children to the render control component on this element
        m_renderControlInterface->Render(renderGraph, this, m_renderInterface, static_cast<int>(m_childElementComponents.size()), isInGame);
    }
    else
    {
        // render any component on this element connected to the UiRenderBus
        if (m_renderInterface)
        {
            m_renderInterface->Render(renderGraph);
        }

        // now render child elements
        int numChildren = static_cast<int>(m_childElementComponents.size());
        for (int i = 0; i < numChildren; ++i)
        {
            GetChildElementComponent(i)->RenderElement(renderGraph, isInGame);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::ElementId UiElementComponent::GetElementId()
{
    return m_elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::NameType UiElementComponent::GetName()
{
    return GetEntity() ? GetEntity()->GetName() : "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetCanvasEntityId()
{
    return m_canvas ? m_canvas->GetEntityId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::GetParent()
{
    return m_parent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetParentEntityId()
{
    return m_parentId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiElementComponent::GetNumChildElements()
{
    return static_cast<int>(m_childEntityIdOrder.size());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::GetChildElement(int index)
{
    AZ::Entity* childEntity = nullptr;
    if (index >= 0 && index < m_childEntityIdOrder.size())
    {
        if (AreChildPointersValid())
        {
            childEntity = GetChildElementComponent(index)->GetEntity();
        }
        else
        {
            AZ::ComponentApplicationBus::BroadcastResult(
                childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_childEntityIdOrder[index].m_entityId);
        }
    }
    return childEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetChildEntityId(int index)
{
    AZ::EntityId childEntityId;
    if (index >= 0 && index < m_childEntityIdOrder.size())
    {
        childEntityId = m_childEntityIdOrder[index].m_entityId;
    }
    return childEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiElementInterface* UiElementComponent::GetChildElementInterface(int index)
{
    return GetChildElementComponent(index);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiElementComponent::GetIndexOfChild(const AZ::Entity* child)
{
    AZ::EntityId childEntityId = child->GetId();
    int numChildren = static_cast<int>(m_childEntityIdOrder.size());
    for (int i = 0;  i < numChildren; ++i)
    {
        if (m_childEntityIdOrder[i].m_entityId == childEntityId)
        {
            return i;
        }
    }
    AZ_Error("UI", false, "The given entity is not a child of this UI element");
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiElementComponent::GetIndexOfChildByEntityId(AZ::EntityId childId)
{
    int numChildren = static_cast<int>(m_childEntityIdOrder.size());
    for (int i = 0;  i < numChildren; ++i)
    {
        if (m_childEntityIdOrder[i].m_entityId == childId)
        {
            return i;
        }
    }
    AZ_Error("UI", false, "The given entity is not a child of this UI element");
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiElementComponent::GetChildElements()
{
    int numChildren = static_cast<int>(m_childEntityIdOrder.size());
    LyShine::EntityArray children;
    children.reserve(numChildren);

    // This is one of the rare functions that needs to work before FixupPostLoad has been called because it is called
    // from OnSliceInstantiated, so only use m_childElementComponents if it is setup
    if (AreChildPointersValid())
    {
        for (int i = 0; i < numChildren; ++i)
        {
            children.push_back(GetChildElementComponent(i)->GetEntity());
        }
    }
    else
    {
        for (auto& childOrderEntry : m_childEntityIdOrder)
        {
            AZ::Entity* childEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, childOrderEntry.m_entityId);
            if (childEntity)
            {
                children.push_back(childEntity);
            }
        }
    }

    return children;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::vector<AZ::EntityId> UiElementComponent::GetChildEntityIds()
{
    AZStd::vector<AZ::EntityId> children;
    for (auto& child : m_childEntityIdOrder)
    {
        children.push_back(child.m_entityId);
    }
    return children;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::CreateChildElement(const LyShine::NameType& name)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    AzFramework::EntityIdContextQueryBus::EventResult(
        contextId, GetEntityId(), &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

    AZ::Entity* child = nullptr;
    UiEntityContextRequestBus::EventResult(child, contextId, &UiEntityContextRequestBus::Events::CreateUiEntity, name.c_str());
    AZ_Assert(child, "Failed to create child entity");

    child->Deactivate();    // deactivate so that we can add components

    UiElementComponent* elementComponent = child->CreateComponent<UiElementComponent>();
    AZ_Assert(elementComponent, "Failed to create UiElementComponent");

    elementComponent->m_canvas = m_canvas;
    elementComponent->SetParentReferences(m_entity, this);
    elementComponent->m_elementId = m_canvas->GenerateId();

    child->Activate();      // re-activate

    if (AreChildPointersValid())    // must test before m_childEntityIdOrder.push_back
    {
        m_childElementComponents.push_back(elementComponent);
    }
    m_childEntityIdOrder.push_back({child->GetId(), m_childEntityIdOrder.size()});

    return child;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::DestroyElement()
{
    PrepareElementForDestroy();

    DestroyElementEntity(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::DestroyElementOnFrameEnd()
{
    PrepareElementForDestroy();

    if (m_canvas)
    {
        // Delay deletion of elements to ensure a script canvas can safely destroy its parent
        m_canvas->ScheduleElementDestroy(GetEntityId());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Reparent(AZ::Entity* newParent, AZ::Entity* insertBefore)
{
    if (!newParent)
    {
        if (IsFullyInitialized())
        {
            newParent = GetCanvasComponent()->GetRootElement();
        }
        else
        {
            EmitNotInitializedWarning();
            return;
        }
    }

    if (newParent == GetEntity())
    {
        AZ_Warning("UI", false, "Cannot set an entity's parent to itself")
        return;
    }

    UiElementComponent* newParentElement = newParent->FindComponent<UiElementComponent>();
    AZ_Assert(newParentElement, "New parent entity has no UiElementComponent");

    // check if the new parent is in a different canvas if so a reparent is not allowed
    // and the caller should do a CloneElement and DestroyElement
    if (m_canvas != newParentElement->m_canvas)
    {
        AZ_Warning("UI", false, "Reparent: Cannot reparent an element to a different canvas");
        return;
    }

    if (m_parent)
    {
        // remove from parent
        GetParentElementComponent()->RemoveChild(GetEntity());
    }

    newParentElement->AddChild(GetEntity(), insertBefore);

    SetParentReferences(newParent, newParentElement);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::ReparentByEntityId(AZ::EntityId newParent, AZ::EntityId insertBefore)
{
    AZ::Entity* newParentEntity = nullptr;
    if (newParent.IsValid())
    {
        AZ::ComponentApplicationBus::BroadcastResult(newParentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, newParent);
        AZ_Assert(newParentEntity, "Entity for newParent not found");
    }

    AZ::Entity* insertBeforeEntity = nullptr;
    if (insertBefore.IsValid())
    {
        AZ::ComponentApplicationBus::BroadcastResult(insertBeforeEntity, &AZ::ComponentApplicationBus::Events::FindEntity, insertBefore);
        AZ_Assert(insertBeforeEntity, "Entity for insertBefore not found");
    }

    Reparent(newParentEntity, insertBeforeEntity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::AddToParentAtIndex(AZ::Entity* newParent, int index)
{
    AZ_Assert(!m_parent, "Element already has a parent");

    if (!newParent)
    {
        if (IsFullyInitialized())
        {
            newParent = GetCanvasComponent()->GetRootElement();
        }
        else
        {
            EmitNotInitializedWarning();
            return;
        }
    }

    UiElementComponent* newParentElement = newParent->FindComponent<UiElementComponent>();
    AZ_Assert(newParentElement, "New parent entity has no UiElementComponent");

    AZ::Entity* insertBefore = nullptr;
    if (index >= 0 && index < newParentElement->GetNumChildElements())
    {
        insertBefore = newParentElement->GetChildElement(index);
    }

    newParentElement->AddChild(GetEntity(), insertBefore);

    SetParentReferences(newParent, newParentElement);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::RemoveFromParent()
{
    if (m_parent)
    {
        // remove from parent
        GetParentElementComponent()->RemoveChild(GetEntity());

        SetParentReferences(nullptr, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindFrontmostChildContainingPoint(AZ::Vector2 point, bool isInGame)
{
    if (!IsFullyInitialized())
    {
        return nullptr;
    }

    AZ::Entity* matchElem = nullptr;

    // this traverses all of the elements in reverse hierarchy order and returns the first one that
    // is containing the point.
    // If necessary, this could be optimized using a spatial partitioning data structure.
    for (int i = static_cast<int>(m_childEntityIdOrder.size() - 1); !matchElem && i >= 0; i--)
    {
        AZ::EntityId child = m_childEntityIdOrder[i].m_entityId;

        if (!isInGame)
        {
            // We are in editing mode (not running the game)
            // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element is
            // hidden in the editor
            bool isVisible = true;
            UiEditorBus::EventResult(isVisible, child, &UiEditorBus::Events::GetIsVisible);
            if (!isVisible)
            {
                continue;
            }
        }

        UiElementComponent* childElementComponent = GetChildElementComponent(i);

        // Check children of this child first
        // child elements do not have to be contained in the parent element's bounds
        matchElem = childElementComponent->FindFrontmostChildContainingPoint(point, isInGame);

        if (!matchElem)
        {
            bool isSelectable = true;
            if (!isInGame)
            {
                // We are in editing mode (not running the game)
                // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element
                // can be selected in the editor
                UiEditorBus::EventResult(isSelectable, child, &UiEditorBus::Events::GetIsSelectable);
            }

            if (isSelectable)
            {
                // if no children of this child matched then check if point is in bounds of this child element
                bool isPointInRect = childElementComponent->GetTransform2dComponent()->IsPointInRect(point);
                if (isPointInRect)
                {
                    matchElem = childElementComponent->GetEntity();
                }
            }
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiElementComponent::FindAllChildrenIntersectingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1, bool isInGame)
{
    LyShine::EntityArray result;

    if (!IsFullyInitialized())
    {
        return result;
    }

    // this traverses all of the elements in hierarchy order
    for (int i = 0; i < m_childEntityIdOrder.size(); ++i)
    {
        AZ::EntityId child = m_childEntityIdOrder[i].m_entityId;

        if (!isInGame)
        {
            // We are in editing mode (not running the game)
            // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element is
            // hidden in the editor
            bool isVisible = true;
            UiEditorBus::EventResult(isVisible, child, &UiEditorBus::Events::GetIsVisible);
            if (!isVisible)
            {
                continue;
            }
        }

        UiElementComponent* childElementComponent = GetChildElementComponent(i);

        // Check children of this child first
        // child elements do not have to be contained in the parent element's bounds
        LyShine::EntityArray childMatches = childElementComponent->FindAllChildrenIntersectingRect(bound0, bound1, isInGame);
        result.insert(result.end(),childMatches.begin(),childMatches.end());

        bool isSelectable = true;
        if (!isInGame)
        {
            // We are in editing mode (not running the game)
            // Use the UiEditorBus to query any UiEditorComponent on this element to see if this element
            // can be selected in the editor
            UiEditorBus::EventResult(isSelectable, child, &UiEditorBus::Events::GetIsSelectable);
        }

        if (isSelectable)
        {
            // check if point is in bounds of this child element
            bool isInRect = childElementComponent->GetTransform2dComponent()->BoundsAreOverlappingRect(bound0, bound1);
            if (isInRect)
            {
                result.push_back(childElementComponent->GetEntity());
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindInteractableToHandleEvent(AZ::Vector2 point)
{
    AZ::EntityId result;

    if (!IsFullyInitialized() || !m_isEnabled)
    {
        // Nothing to do
        return result;
    }

    // first check the children (in reverse order) since children are in front of parent
    {
        // if this element is masking children at this point then don't check the children
        bool isMasked = false;
        UiInteractionMaskBus::EventResult(isMasked, GetEntityId(), &UiInteractionMaskBus::Events::IsPointMasked, point);
        if (!isMasked)
        {
            for (int i = static_cast<int>(m_childEntityIdOrder.size() - 1); !result.IsValid() && i >= 0; i--)
            {
                result = GetChildElementComponent(i)->FindInteractableToHandleEvent(point);
            }
        }
    }

    // if no match then check this element
    if (!result.IsValid())
    {
        // if this element has an interactable component and the point is in this element's rect
        if (UiInteractableBus::FindFirstHandler(GetEntityId()))
        {
            bool isInRect = GetTransform2dComponent()->IsPointInRect(point);
            if (isInRect)
            {
                // check if this interactable component is in a state where it can handle an event at the given point
                bool canHandle = false;
                UiInteractableBus::EventResult(canHandle, GetEntityId(), &UiInteractableBus::Events::CanHandleEvent, point);
                if (canHandle)
                {
                    result = GetEntityId();
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindParentInteractableSupportingDrag(AZ::Vector2 point)
{
    AZ::EntityId result;

    // if this element has a parent and this element is not completely disabled
    if (m_parent && m_isEnabled)
    {
        AZ::EntityId parentEntity = m_parent->GetId();

        // if the parent supports drag hand off then return it
        bool supportsDragOffset = false;
        UiInteractableBus::EventResult(supportsDragOffset, parentEntity, &UiInteractableBus::Events::DoesSupportDragHandOff, point);
        if (supportsDragOffset)
        {
            // Make sure the parent is also handling events
            bool handlingEvents = false;
            UiInteractableBus::EventResult(handlingEvents, parentEntity, &UiInteractableBus::Events::IsHandlingEvents);
            supportsDragOffset = handlingEvents;
        }

        if (supportsDragOffset)
        {
            result = parentEntity;
        }
        else
        {
            // else keep going up the parent links
            result = GetParentElementComponent()->FindParentInteractableSupportingDrag(point);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindChildByName(const LyShine::NameType& name)
{
    AZ::Entity* matchElem = nullptr;

    if (AreChildPointersValid())
    {
        int numChildren = static_cast<int>(m_childElementComponents.size());
        for (int i = 0; i < numChildren; ++i)
        {
            AZ::Entity* childEntity = GetChildElementComponent(i)->GetEntity();
            if (name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }
        }
    }
    else
    {
        for (auto& child : m_childEntityIdOrder)
        {
            AZ::Entity* childEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, child.m_entityId);
            if (childEntity && name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindDescendantByName(const LyShine::NameType& name)
{
    AZ::Entity* matchElem = nullptr;

    if (AreChildPointersValid())
    {
        int numChildren = static_cast<int>(m_childElementComponents.size());
        for (int i = 0; i < numChildren; ++i)
        {
            UiElementComponent* childElementComponent = GetChildElementComponent(i);
            AZ::Entity* childEntity = childElementComponent->GetEntity();

            if (name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }

            matchElem = childElementComponent->FindDescendantByName(name);
            if (matchElem)
            {
                break;
            }
        }
    }
    else
    {
        for (auto& child : m_childEntityIdOrder)
        {
            AZ::Entity* childEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, child.m_entityId);

            if (childEntity && name == childEntity->GetName())
            {
                matchElem = childEntity;
                break;
            }

            UiElementBus::EventResult(matchElem, child.m_entityId, &UiElementBus::Events::FindDescendantByName, name);
            if (matchElem)
            {
                break;
            }
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindChildEntityIdByName(const LyShine::NameType& name)
{
    AZ::Entity* childEntity = FindChildByName(name);
    return childEntity ? childEntity->GetId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::FindDescendantEntityIdByName(const LyShine::NameType& name)
{
    AZ::Entity* childEntity = FindDescendantByName(name);
    return childEntity ? childEntity->GetId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindChildByEntityId(AZ::EntityId id)
{
    AZ::Entity* matchElem = nullptr;

    int numChildren = static_cast<int>(m_childEntityIdOrder.size());
    for (int i = 0; i < numChildren; ++i)
    {
        if (id == m_childEntityIdOrder[i].m_entityId)
        {
            if (AreChildPointersValid())
            {
                matchElem = GetChildElementComponent(i)->GetEntity();
            }
            else
            {
                AZ::ComponentApplicationBus::BroadcastResult(matchElem, &AZ::ComponentApplicationBus::Events::FindEntity, id);
            }
            break;
        }
    }

    return matchElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiElementComponent::FindDescendantById(LyShine::ElementId id)
{
    if (id == m_elementId)
    {
        return GetEntity();
    }

    AZ::Entity* match = nullptr;

    if (AreChildPointersValid())
    {
        int numChildren = static_cast<int>(m_childEntityIdOrder.size());
        for (int i = 0; !match && i < numChildren; ++i)
        {
            match = GetChildElementComponent(i)->FindDescendantById(id);
        }
    }
    else
    {
        for (auto iter = m_childEntityIdOrder.begin(); !match && iter != m_childEntityIdOrder.end(); iter++)
        {
            UiElementBus::EventResult(match, iter->m_entityId, &UiElementBus::Events::FindDescendantById, id);
        }
    }

    return match;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::FindDescendantElements(AZStd::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result)
{
    if (AreChildPointersValid())
    {
        int numChildren = static_cast<int>(m_childElementComponents.size());
        for (int i = 0; i < numChildren; ++i)
        {
            UiElementComponent* childElementComponent = GetChildElementComponent(i);

            AZ::Entity* childEntity = childElementComponent->GetEntity();
            if (predicate(childEntity))
            {
                result.push_back(childEntity);
            }

            childElementComponent->FindDescendantElements(predicate, result);
        }
    }
    else
    {
        for (auto& child : m_childEntityIdOrder)
        {
            AZ::Entity* childEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, child.m_entityId);
            if (childEntity && predicate(childEntity))
            {
                result.push_back(childEntity);
            }

            UiElementBus::Event(child.m_entityId, &UiElementBus::Events::FindDescendantElements, predicate, result);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::CallOnDescendantElements(AZStd::function<void(const AZ::EntityId)> callFunction)
{
    if (AreChildPointersValid())
    {
        int numChildren = static_cast<int>(m_childEntityIdOrder.size());
        for (int i = 0; i < numChildren; ++i)
        {
            callFunction(m_childEntityIdOrder[i].m_entityId);

            GetChildElementComponent(i)->CallOnDescendantElements(callFunction);
        }
    }
    else
    {
        for (auto& child : m_childEntityIdOrder)
        {
            callFunction(child.m_entityId);
            UiElementBus::Event(child.m_entityId, &UiElementBus::Events::CallOnDescendantElements, callFunction);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::IsAncestor(AZ::EntityId id)
{
    UiElementComponent* parentElementComponent = GetParentElementComponent();
    while (parentElementComponent)
    {
        if (parentElementComponent->GetEntityId() == id)
        {
            return true;
        }

        parentElementComponent = parentElementComponent->GetParentElementComponent();
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::IsEnabled()
{
    return m_isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsEnabled(bool isEnabled)
{
    if (isEnabled != m_isEnabled)
    {
        m_isEnabled = isEnabled;

        // Tell any listeners that the enabled state has changed
        UiElementNotificationBus::Event(GetEntityId(), &UiElementNotificationBus::Events::OnUiElementEnabledChanged, m_isEnabled);

        // If the ancestors are not enabled then changing the local flag has no effect on the effective
        // enabled state.
        bool areAncestorsEnabled = (m_parentElementComponent) ? m_parentElementComponent->GetAreElementAndAncestorsEnabled() : true;
        if (areAncestorsEnabled)
        {
            // Tell any listeners that the effective enabled state has changed
            UiElementNotificationBus::Event(
                GetEntityId(), &UiElementNotificationBus::Events::OnUiElementAndAncestorsEnabledChanged, m_isEnabled);

            DoRecursiveEnabledNotification(m_isEnabled);
        }

        // tell the canvas to invalidate the render graph
        if (m_canvas)
        {
            m_canvas->MarkRenderGraphDirty();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetAreElementAndAncestorsEnabled()
{
    if (!m_isEnabled)
    {
        return false;
    }

    if (m_parentElementComponent)
    {
        return m_parentElementComponent->GetAreElementAndAncestorsEnabled();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::IsRenderEnabled()
{
    return m_isRenderEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsRenderEnabled(bool isRenderEnabled)
{
    m_isRenderEnabled = isRenderEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsVisible()
{
    return m_isVisibleInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsVisible(bool isVisible)
{
    if (m_isVisibleInEditor != isVisible)
    {
        m_isVisibleInEditor = isVisible;

        if (m_canvas)
        {
            // we have to regenerate the graph because different elements are now visible
            m_canvas->MarkRenderGraphDirty();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsSelectable()
{
    return m_isSelectableInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsSelectable(bool isSelectable)
{
    m_isSelectableInEditor = isSelectable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsSelected()
{
    return m_isSelectedInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsSelected(bool isSelected)
{
    m_isSelectedInEditor = isSelected;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::GetIsExpanded()
{
    return m_isExpandedInEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetIsExpanded(bool isExpanded)
{
    m_isExpandedInEditor = isExpanded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::AreAllAncestorsVisible()
{
    UiElementComponent* parentElementComponent = GetParentElementComponent();
    while (parentElementComponent)
    {
        bool isParentVisible = true;
        UiEditorBus::EventResult(isParentVisible, parentElementComponent->GetEntityId(), &UiEditorBus::Events::GetIsVisible);
        if (!isParentVisible)
        {
            return false;
        }

        // Walk up the hierarchy.
        parentElementComponent = parentElementComponent->GetParentElementComponent();
    }

    // there is no ancestor entity that is not visible
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::OnEntityActivated(const AZ::EntityId&)
{
    // cache pointers to the optional render interface and render control interface to
    // optimize calls rather than using ebus. Both of these buses only allow single handlers.
    m_renderInterface = UiRenderBus::FindFirstHandler(GetEntityId());
    m_renderControlInterface = UiRenderControlBus::FindFirstHandler(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::OnEntityDeactivated(const AZ::EntityId&)
{
    m_renderInterface = nullptr;
    m_renderControlInterface = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::AddChild(AZ::Entity* child, AZ::Entity* insertBefore)
{
    // debug check that this element is not already a child
    AZ_Assert(FindChildByEntityId(child->GetId()) == nullptr, "Attempting to add a duplicate child");

    UiElementComponent* childElementComponent = child->FindComponent<UiElementComponent>();
    AZ_Assert(childElementComponent, "Attempting to add a child with no element component");
    if (!childElementComponent)
    {
        return;
    }

    bool wasInserted = false;

    if (insertBefore)
    {
        int numChildren = static_cast<int>(m_childEntityIdOrder.size());
        for (int i = 0; i < numChildren; ++i)
        {
            if (m_childEntityIdOrder[i].m_entityId == insertBefore->GetId())
            {
                if (AreChildPointersValid())    // must test before m_childEntityIdOrder.insert
                {
                    m_childElementComponents.insert(m_childElementComponents.begin() + i, childElementComponent);
                }

                m_childEntityIdOrder.insert(m_childEntityIdOrder.begin() + i, {child->GetId(), static_cast<AZ::u64>(i)});

                ResetChildEntityIdSortOrders();

                wasInserted = true;
                break;
            }
        }
    }

    // either insertBefore is null or it is not found, insert at end
    if (!wasInserted)
    {
        if (AreChildPointersValid())    // must test before m_childEntityIdOrder.push_back
        {
            m_childElementComponents.push_back(childElementComponent);
        }
        m_childEntityIdOrder.push_back({child->GetId(), m_childEntityIdOrder.size()});
    }

    // Adding or removing child elements may require recomputing the
    // transforms of all children
    UiLayoutManagerBus::Event(GetCanvasEntityId(), &UiLayoutManagerBus::Events::MarkToRecomputeLayout, GetEntityId());
    UiLayoutManagerBus::Event(
        GetCanvasEntityId(), &UiLayoutManagerBus::Events::MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), false);

    // It will always require recomputing the transform for the child just added
    if (IsFullyInitialized())
    {
        GetTransform2dComponent()->SetRecomputeFlags(UiTransformInterface::Recompute::RectAndTransform);
    }

    // tell the canvas to invalidate the render graph
    if (m_canvas)
    {
        m_canvas->MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::RemoveChild(AZ::Entity* child)
{
    // check if the given entity is actually a child, if not then do nothing
    AZ::EntityId childId = child->GetId();
    auto iter = AZStd::find_if(m_childEntityIdOrder.begin(), m_childEntityIdOrder.end(),
        [childId](ChildEntityIdOrderEntry& entry) { return entry.m_entityId == childId; });

    if (iter != m_childEntityIdOrder.end())
    {
        // remove the child from m_childEntityIdOrder
        m_childEntityIdOrder.erase(iter);

        // update the sort indices to be contiguous
        ResetChildEntityIdSortOrders();

        UiElementComponent* elementComponent = child->FindComponent<UiElementComponent>();
        AZ_Assert(elementComponent, "Child element has no UiElementComponent");

        // Also erase from m_childElementComponents
        stl::find_and_erase(m_childElementComponents, elementComponent);

        // Clear child's parent
        elementComponent->SetParentReferences(nullptr, nullptr);

        // Adding or removing child elements may require recomputing the
        // transforms of all children
        UiLayoutManagerBus::Event(GetCanvasEntityId(), &UiLayoutManagerBus::Events::MarkToRecomputeLayout, GetEntityId());
        UiLayoutManagerBus::Event(
            GetCanvasEntityId(), &UiLayoutManagerBus::Events::MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), false);

        // tell the canvas to invalidate the render graph
        if (m_canvas)
        {
            m_canvas->MarkRenderGraphDirty();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::RemoveChild(AZ::EntityId child)
{
    AZ::EntityId childId = child;
    auto iter = AZStd::find_if(m_childEntityIdOrder.begin(), m_childEntityIdOrder.end(),
        [childId](ChildEntityIdOrderEntry& entry) { return entry.m_entityId == childId; });

    if (iter != m_childEntityIdOrder.end())
    {
        if (AreChildPointersValid())
        {
            auto childElementiter = AZStd::find_if(m_childElementComponents.begin(), m_childElementComponents.end(),
                [childId](UiElementComponent* elementComponent) { return elementComponent->GetEntityId() == childId; });

            UiElementComponent* elementComponent = childElementiter != m_childElementComponents.end() ? *childElementiter : nullptr;
            AZ_Assert(elementComponent, "");
            if (elementComponent)
            {
                stl::find_and_erase(m_childElementComponents, elementComponent);

                // Clear child's parent
                elementComponent->SetParentReferences(nullptr, nullptr);
            }
        }

        // remove the child from m_childEntityIdOrder
        m_childEntityIdOrder.erase(iter);

        // update the sort indicies to be contiguous
        ResetChildEntityIdSortOrders();

        // Adding or removing child elements may require recomputing the
        // transforms of all children
        UiLayoutManagerBus::Event(GetCanvasEntityId(), &UiLayoutManagerBus::Events::MarkToRecomputeLayout, GetEntityId());
        UiLayoutManagerBus::Event(
            GetCanvasEntityId(), &UiLayoutManagerBus::Events::MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), false);

        // tell the canvas to invalidate the render graph
        if (m_canvas)
        {
            m_canvas->MarkRenderGraphDirty();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetCanvas(UiCanvasComponent* canvas, LyShine::ElementId elementId)
{
    m_canvas = canvas;
    m_elementId = elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::FixupPostLoad(AZ::Entity* entity, UiCanvasComponent* canvas, AZ::Entity* parent, bool makeNewElementIds)
{
#ifdef AZ_DEBUG_BUILD
    // check that the m_childEntityIdOrder is ordered such that the m_sortIndex fields are in order and contiguous
    {
        size_t numChildren = m_childEntityIdOrder.size();
        for (size_t index = 0; index < numChildren; ++index)
        {
            if (m_childEntityIdOrder[index].m_sortIndex != index)
            {
                AZ_Assert(false, "FixupPostLoad: m_childEntityIdOrder bad sort index. This should never happen.");
            }
        }
    }
#endif

    if (makeNewElementIds)
    {
        m_elementId = canvas->GenerateId();
    }

    m_canvas = canvas;

    if (parent)
    {
        UiElementComponent* parentElementComponent = parent->FindComponent<UiElementComponent>();
        AZ_Assert(parentElementComponent, "Parent element has no UiElementComponent");
        SetParentReferences(parent, parentElementComponent);
    }
    else
    {
        SetParentReferences(nullptr, nullptr);
    }

    ChildEntityIdOrderArray missingChildren;

    for (auto& child : m_childEntityIdOrder)
    {
        AZ::Entity* childEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, child.m_entityId);
        if (!childEntity)
        {
            // with slices it is possible for users to get themselves into situations where a child no
            // longer exists, we should report an error in this case rather than asserting
            AZ_Error("UI", false, "Child element with Entity ID %llu no longer exists. Data will be lost.", child);
            // This case could happen if a slice asset has been deleted. We should try to continue and load the
            // canvas with errors.
            missingChildren.push_back(child);
            continue;
        }

        UiElementComponent* elementComponent = childEntity->FindComponent<UiElementComponent>();
        if (!elementComponent)
        {
            // with slices it is possible for users to get themselves into situations where a child no
            // longer has an element component. In this case report an error and fail to load the data but do not
            // crash.
            AZ_Error("UI", false, "Child element with Entity ID %llu no longer has a UiElementComponent. Data cannot be loaded.", child);
            return false;
        }

        bool success = elementComponent->FixupPostLoad(childEntity, canvas, entity, makeNewElementIds);
        if (!success)
        {
            return false;
        }
    }

    // If there were any missing children remove them from the m_childEntityIdOrder list
    // This is recovery code for the case that a slice asset that we were using has been removed.
    for (auto child : missingChildren)
    {
        stl::find_and_erase(m_childEntityIdOrder, child);
    }

    // Initialize the m_childElementComponents array that is used for performance optimization
    m_childElementComponents.clear();
    for (auto child : m_childEntityIdOrder)
    {
        AZ::Entity* childEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, child.m_entityId);
        AZ_Assert(childEntity, "Child element not found");
        UiElementComponent* childElementComponent = childEntity->FindComponent<UiElementComponent>();
        AZ_Assert(childElementComponent, "Child element has no UiElementComponent");
        m_childElementComponents.push_back(childElementComponent);
    }

    // Tell any listeners that the canvas entity ID for the element is now set, this allows other components to
    // listen for messages from the canvas
    AZ::EntityId parentEntityId = (parent) ? parent->GetId() : AZ::EntityId();
    UiElementNotificationBus::Event(
        GetEntityId(), &UiElementNotificationBus::Events::OnUiElementFixup, canvas->GetEntityId(), parentEntityId);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiElementComponent::GetSliceEntityParentId()
{
    return GetParentEntityId();
}

AZStd::vector<AZ::EntityId> UiElementComponent::GetSliceEntityChildren()
{
    return GetChildEntityIds();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiElementComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<ChildEntityIdOrderEntry>()
            // Persistent IDs for this are simply the entity id
            ->PersistentId([](const void* instance) -> AZ::u64
            {
                const ChildEntityIdOrderEntry* entry = reinterpret_cast<const ChildEntityIdOrderEntry*>(instance);
                return static_cast<AZ::u64>(entry->m_entityId);
            })
            ->Version(1)
            ->Field("ChildEntityId", &ChildEntityIdOrderEntry::m_entityId)
            ->Field("SortIndex", &ChildEntityIdOrderEntry::m_sortIndex);

        serializeContext->Class<UiElementComponent, AZ::Component>()
            ->Version(3, &VersionConverter)
            ->EventHandler<ChildOrderSerializationEvents>()
            ->Field("Id", &UiElementComponent::m_elementId)
            ->Field("IsEnabled", &UiElementComponent::m_isEnabled)
            ->Field("IsVisibleInEditor", &UiElementComponent::m_isVisibleInEditor)
            ->Field("IsSelectableInEditor", &UiElementComponent::m_isSelectableInEditor)
            ->Field("IsSelectedInEditor", &UiElementComponent::m_isSelectedInEditor)
            ->Field("IsExpandedInEditor", &UiElementComponent::m_isExpandedInEditor)
            ->Field("ChildEntityIdOrder", &UiElementComponent::m_childEntityIdOrder);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiElementComponent>("Element", "Adds UI Element behavior to an entity");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiElement.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiElement.png")
                ->Attribute(AZ::Edit::Attributes::AddableByUser, false)     // Cannot be added or removed by user
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("String", &UiElementComponent::m_elementId, "Id",
                "This read-only ID is used to reference the element from FlowGraph")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable);

            editInfo->DataElement(0, &UiElementComponent::m_isEnabled, "Start enabled",
                "Determines whether the element is enabled upon creation.\n"
                "If an element is not enabled, neither it nor any of its children are drawn or interactive.");

            // These are not visible in the PropertyGrid since they are managed through the Hierarchy Pane
            // We do want to be able to push them to a slice though.
            editInfo->DataElement(0, &UiElementComponent::m_isVisibleInEditor, "IsVisibleInEditor", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide);
            editInfo->DataElement(0, &UiElementComponent::m_isSelectableInEditor, "IsSelectableInEditor", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide);
            editInfo->DataElement(0, &UiElementComponent::m_isExpandedInEditor, "IsExpandedInEditor", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiElementBus>("UiElementBus")
            ->Event("GetName", &UiElementBus::Events::GetName)
            ->Event("GetCanvas", &UiElementBus::Events::GetCanvasEntityId)
            ->Event("GetParent", &UiElementBus::Events::GetParentEntityId)
            ->Event("GetNumChildElements", &UiElementBus::Events::GetNumChildElements)
            ->Event("GetChild", &UiElementBus::Events::GetChildEntityId)
            ->Event("GetIndexOfChildByEntityId", &UiElementBus::Events::GetIndexOfChildByEntityId)
            ->Event("GetChildren", &UiElementBus::Events::GetChildEntityIds)
            ->Event("DestroyElement", &UiElementBus::Events::DestroyElementOnFrameEnd)
            ->Event("Reparent", &UiElementBus::Events::ReparentByEntityId)
            ->Event("FindChildByName", &UiElementBus::Events::FindChildEntityIdByName)
            ->Event("FindDescendantByName", &UiElementBus::Events::FindDescendantEntityIdByName)
            ->Event("IsAncestor", &UiElementBus::Events::IsAncestor)
            ->Event("IsEnabled", &UiElementBus::Events::IsEnabled)
            ->Event("SetIsEnabled", &UiElementBus::Events::SetIsEnabled);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Initialize()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::MoveEntityAndDescendantsToListAndReplaceWithEntityId(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& elementNode,
    int index,
    AZStd::vector<AZ::SerializeContext::DataElementNode>& entities)
{
    // Find the UiElementComponent on this entity
    AZ::SerializeContext::DataElementNode* elementComponentNode =
        LyShine::FindComponentNode(elementNode, UiElementComponent::TYPEINFO_Uuid());
    if (!elementComponentNode)
    {
        return false;
    }

    // We must process the children first so that when we make a copy of this entity to the entities list
    // it will already have had its child entities replaced with entity IDs

    // find the m_children field
    int childrenIndex = elementComponentNode->FindElement(AZ_CRC_CE("Children"));
    if (childrenIndex == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode& childrenNode = elementComponentNode->GetSubElement(childrenIndex);

    // Create the child entities member (which is a generic vector)
    AZ::SerializeContext::ClassData* classData = AZ::SerializeGenericTypeInfo<ChildEntityIdOrderArray>::GetGenericInfo()->GetClassData();
    int newChildrenIndex = elementComponentNode->AddElement(context, "ChildEntityIdOrder", *classData);
    if (newChildrenIndex == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode& newChildrenNode = elementComponentNode->GetSubElement(newChildrenIndex);

    // iterate through children and recursively call this function
    int numChildren = childrenNode.GetNumSubElements();
    for (int childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        AZ::SerializeContext::DataElementNode& childElementNode = childrenNode.GetSubElement(childIndex);
        MoveEntityAndDescendantsToListAndReplaceWithEntityId(context, childElementNode, childIndex, entities);

        newChildrenNode.AddElement(childElementNode);
    }

    // delete the original "Children" node, we have replaced it with the "ChildEntityIdOrder" node
    elementComponentNode->RemoveElement(childrenIndex);

    // the children list has now been processed so it will now just contain entity IDs
    // Now copy this node (elementNode) to the list we are building and then replace it
    // with an Entity ID node

    // copy this node to the list
    entities.push_back(elementNode);

    // Remember the name of this node (it could be "element" or "RootElement" for example)
    AZStd::string elementFieldName = elementNode.GetNameString();

    // Find the EntityId node within this entity
    int entityIdIndex = elementNode.FindElement(AZ_CRC_CE("Id"));
    if (entityIdIndex == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode& elementIdNode = elementNode.GetSubElement(entityIdIndex);

    // Find the sub node of the EntityID that actually stores the u64 and make a copy of it
    int u64Index = elementIdNode.FindElement(AZ_CRC_CE("id"));
    if (u64Index == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode u64Node = elementIdNode.GetSubElement(u64Index);

    // -1 indicates this is the root element reference
    if (index == -1)
    {
        // Convert this node (which was an entire Entity) into just an EntityId, keeping the same
        // node name as it had
        elementNode.Convert<AZ::EntityId>(context, elementFieldName.c_str());

        // copy in the subNode that stores the actual u64 (that we saved a copy of above)
        elementNode.AddElement(u64Node);
    }
    else
    {
        // Convert this node (which was an entire Entity) into just an ChildEntityIdOrderEntry, keeping the same
        // node name as it had
        elementNode.Convert<ChildEntityIdOrderEntry>(context, elementFieldName.c_str());

        // add sub element from the entity Id
        int childOrderEntryEntityIdIndex = elementNode.AddElement<AZ::EntityId>(context, "ChildEntityId");
        AZ::SerializeContext::DataElementNode& childOrderEntryEntityIdElementNode = elementNode.GetSubElement(childOrderEntryEntityIdIndex);

        // copy in the subNode that stores the actual u64 (that we saved a copy of above)
        childOrderEntryEntityIdElementNode.AddElement(u64Node);

        AZ::u64 sortIndex = static_cast<AZ::u64>(index);
        elementNode.AddElementWithData<AZ::u64>(context, "SortIndex", sortIndex);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Activate()
{
    UiElementBus::Handler::BusConnect(m_entity->GetId());
    UiEditorBus::Handler::BusConnect(m_entity->GetId());
    AZ::SliceEntityHierarchyRequestBus::Handler::BusConnect(m_entity->GetId());
    AZ::EntityBus::Handler::BusConnect(m_entity->GetId());

    // Once added the transform component is never removed
    if (!m_transformComponent)
    {
        m_transformComponent = GetEntity()->FindComponent<UiTransform2dComponent>();
    }

    // tell the canvas to invalidate the render graph
    if (m_canvas)
    {
        m_canvas->MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::Deactivate()
{
    UiElementBus::Handler::BusDisconnect();
    UiEditorBus::Handler::BusDisconnect();
    AZ::SliceEntityHierarchyRequestBus::Handler::BusDisconnect();
    AZ::EntityBus::Handler::BusDisconnect();

    // tell the canvas to invalidate the render graph
    if (m_canvas)
    {
        m_canvas->MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::DoRecursiveEnabledNotification(bool newIsEnabledValue)
{
    for (UiElementComponent* child : m_childElementComponents)
    {
        // if this child element is disabled then the enabled state of the ancestors makes no difference
        // but if it is enabled then its effective enabled state is controlled by its ancestors
        if (child->m_isEnabled)
        {
            UiElementNotificationBus::Event(
                child->GetEntityId(), &UiElementNotificationBus::Events::OnUiElementAndAncestorsEnabledChanged, newIsEnabledValue);
            child->DoRecursiveEnabledNotification(newIsEnabledValue);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::EmitNotInitializedWarning() const
{
    AZ_Warning("UI", false, "UiElementComponent used before fully initialized, possibly on activate before FixupPostLoad was called on this element")
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::SetParentReferences(AZ::Entity* parent, UiElementComponent* parentElementComponent)
{
    m_parent = parent;
    m_parentId = (parent) ? parent->GetId() : AZ::EntityId();
    m_parentElementComponent = parentElementComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::OnPatchEnd(const AZ::DataPatchNodeInfo& patchInfo)
{
    // We want to check the data patch for any patching of the "Children" element. The m_children element no
    // longer exists so we want to make the equivalent changes to the m_childEntityIdOrder element.

    // The relevant patch addresses can be either
    // a) a change of an element in the container
    // b) a removal of an element in the container (these are always higher indices than the changes)
    // c) an addition of an element in the container (these are not always in ascending order, it is an unordered map)
    //    (these are always higher indices than the changes)
    //
    // For a given patch there will never be both addition and removals.
    //
    // For b and c the patch address (in childPatchLookup) will be patchinfo.address + "Children".
    // We could find all of those through one call to "find" on childPatchLookup with that address.
    // However, for the "a" case the address (in childPatchLookup) will have an additional element on
    // the end - since it is the "Id" field within the EntityId that is being patched.
    // So we have to iterate through childPatchLookup anyway, so we do that for all cases.

    using EntityIndexPair = AZStd::pair<AZ::u64, AZ::EntityId>;
    using EntityIndexPairList = AZStd::vector<EntityIndexPair>;

    EntityIndexPairList elementsChanged;
    EntityIndexPairList elementsAdded;
    AZStd::vector<AZ::u64> elementsRemoved;
    bool oldChildrenDataPatchFound = false;

    const AZ::DataPatch::AddressType& address = patchInfo.address;
    const AZ::DataPatch::PatchMap& patch = patchInfo.patch;
    const AZ::DataPatch::ChildPatchMap& childPatchLookup = patchInfo.childPatchLookup;

    // Build the address of the "Children" element within this UiElementComponent
    AZ::DataPatch::AddressType childrenAddress = address;
    childrenAddress.push_back(AZ_CRC_CE("Children"));

    // Get the serialize context for use in the LoadObjectFromStreamInPlace calls
    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

    // childPatchLookup contains all addresses in the patch that are within the UiElementComponent so
    // it is slightly faster to iterate over that than over "patch" directly.
    for (auto& childPatchPair : childPatchLookup)
    {
        const AZ::DataPatch::AddressType& lookupAddress = childPatchPair.first;

        if (lookupAddress == childrenAddress)
        {
            // The address matches the "Children" container exactly, so get childPatches which will contain
            // all the additions and removals to the container.
            const AZStd::vector<AZ::DataPatch::AddressType>& childPatches = childPatchPair.second;

            for (auto& childPatchAddress : childPatches)
            {
                auto foundPatchIt = patch.find(childPatchAddress);

                if (foundPatchIt == patch.end())
                {
                    // this should never happen, ignore it if it does
                    continue;
                }

                // the last part of the address is the index in the m_children array
                AZ::u64 index = childPatchAddress.back().GetAddressElement();

                if (foundPatchIt->second.empty())
                {
                    // this is removal of element (actual patch is empty)
                    oldChildrenDataPatchFound = true;
                    elementsRemoved.push_back(index);
                }
                else
                {
                    // This is an addition

                    // get the EntityId out of the patch value
                    AZ::EntityId entityId;
                    bool entityIdLoaded = false;

                    // If the patch originated in a Legacy DataPatch then we must first load the EntityId from the legacy stream
                    if (foundPatchIt->second.type() == azrtti_typeid<AZ::DataPatch::LegacyStreamWrapper>())
                    {
                        const AZ::DataPatch::LegacyStreamWrapper* wrapper = AZStd::any_cast<AZ::DataPatch::LegacyStreamWrapper>(&foundPatchIt->second);

                        if (wrapper)
                        {
                            AZ::IO::MemoryStream stream(wrapper->m_stream.data(), wrapper->m_stream.size());
                            entityIdLoaded = AZ::Utils::LoadObjectFromStreamInPlace<AZ::EntityId>(stream, entityId, serializeContext);
                        }
                    }
                    else
                    {
                        // Otherwise we can acquire the EntityId from the patch directly
                        const AZ::EntityId* entityIdPtr = AZStd::any_cast<AZ::EntityId>(&foundPatchIt->second);

                        if (entityIdPtr)
                        {
                            entityId = *entityIdPtr;
                            entityIdLoaded = true;
                        }
                    }

                    if (entityIdLoaded)
                    {
                        oldChildrenDataPatchFound = true;
                        elementsAdded.push_back({index, entityId});
                    }
                    else
                    {
                        AZ_Error("UI", false, "UiElement::OnPatchEnd: Failed to load a child entity Id from DataPatch");
                    }
                }
            }
        }
        else if (lookupAddress.size() == childrenAddress.size() + 1)
        {
            // the lookupAddress is the same length as the "Children" address plus an index
            // check if the address is childrenAddress plus an extra element
            bool match = true;
            for (int i = static_cast<int>(childrenAddress.size() - 1); i >= 0; --i)
            {
                if (lookupAddress[i] != childrenAddress[i])
                {
                    match = false;
                    break;
                }
            }

            if (!match)
            {
                continue;
            }

            // childPatches will be any patches to this one element in the children array (should only ever be one element in the map)
            const AZStd::vector<AZ::DataPatch::AddressType>& childPatches = childPatchPair.second;

            for (auto& childPatchAddress : childPatches)
            {
                auto foundPatchIt = patch.find(childPatchAddress);

                if (foundPatchIt == patch.end())
                {
                    // this should never happen, ignore it if it does
                    continue;
                }

                if (foundPatchIt->second.empty())
                {
                    // this is removal of element (actual patch is empty). Should never occur in this path. Ignore.
                    continue;
                }

                // This should be the u64 "Id" element of the EntityId, if not ignore.
                if (childPatchAddress.back().GetAddressElement() == AZ_CRC_CE("Id"))
                {
                    // the second to last part of the address is the index in the m_children array
                    AZ::u64 index = childPatchAddress[childPatchAddress.size() - 2].GetAddressElement();

                    // extract the u64 from the patch value
                    AZ::u64 id = 0;
                    bool idLoaded = false;

                    // If the patch originated in a Legacy DataPatch then we must first load the u64 from the legacy stream
                    if (foundPatchIt->second.type() == azrtti_typeid<AZ::DataPatch::LegacyStreamWrapper>())
                    {
                        const AZ::DataPatch::LegacyStreamWrapper* wrapper = AZStd::any_cast<AZ::DataPatch::LegacyStreamWrapper>(&foundPatchIt->second);

                        if (wrapper)
                        {
                            AZ::IO::MemoryStream stream(wrapper->m_stream.data(), wrapper->m_stream.size());

                            idLoaded = AZ::Utils::LoadObjectFromStreamInPlace<AZ::u64>(stream, id, serializeContext);
                        }
                    }
                    else
                    {
                        // Otherwise we can acquire the EntityId from the patch directly
                        const AZ::u64* idPtr = AZStd::any_cast<AZ::u64>(&foundPatchIt->second);

                        if (idPtr)
                        {
                            id = *idPtr;
                            idLoaded = true;
                        }
                    }

                    if (idLoaded)
                    {
                        AZ::EntityId entityId(id);
                        oldChildrenDataPatchFound = true;
                        elementsChanged.push_back({index, entityId});
                    }
                    else
                    {
                        AZ_Error("UI", false, "UiElement::OnPatchEnd: Failed to load a child entity Id from DataPatch");
                    }
                }
            }
        }
    }

    // if patch data for the old "Children" container was found then apply it to the new m_childEntityIdOrder vector
    if (oldChildrenDataPatchFound)
    {
        if (elementsAdded.size() > 0 && elementsRemoved.size() > 0)
        {
            AZ_Error("UI", false, "OnPatchEnd: can't add and remove in the same patch");
        }

        // removing elements always removes from the end. So we just need to resize to the lowest index
        for (AZ::u64 index : elementsRemoved)
        {
            if (index < m_childEntityIdOrder.size())
            {
                m_childEntityIdOrder.resize(index);
            }
        }

        for (auto& elementChanged : elementsChanged)
        {
            AZ::u64 index = elementChanged.first;
            if (index < m_childEntityIdOrder.size())
            {
                m_childEntityIdOrder[index].m_entityId = elementChanged.second;
            }
            else
            {
                // index is off the end of m_childEntityIdOrder, this can happen because
                // elements could be been removed from the slice. But since this override has changed
                // the entityId we do not want to remove it. So add at end.
                m_childEntityIdOrder.push_back({elementChanged.second, m_childEntityIdOrder.size()});
            }
        }

        // sort the added elements by index
        AZStd::sort(elementsAdded.begin(), elementsAdded.end());
        for (auto& elementAdded : elementsAdded)
        {
            // elements could have been added or removed in the slice so we don't require that there must be an element 3
            // to add element 4, if not we just add it at the end.
            m_childEntityIdOrder.push_back({elementAdded.second, m_childEntityIdOrder.size()});
        }
   }

    // regardless of whether the old m_children was in the patch we always sort m_childEntityIdOrder and reassign sort indices after
    // patching to maintain a consecutive set of sort indices

    // This will sort all the entity order entries by sort index (primary) and entity id (secondary) which should never result in any collisions
    // This is used since slice data patching may create duplicate entries for the same sort index, missing indices and the like.
    // It should never result in multiple entity id entries since the serialization of this data uses a persistent id which is the entity id
    int numChildren = static_cast<int>(m_childEntityIdOrder.size());
    if (numChildren > 0)
    {
        AZStd::sort(m_childEntityIdOrder.begin(), m_childEntityIdOrder.end());
        ResetChildEntityIdSortOrders();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::ResetChildEntityIdSortOrders()
{
    // Set the sortIndex on each child to match the order in the vector
    for (AZ::u64 childIndex = 0; childIndex < m_childEntityIdOrder.size(); ++childIndex)
    {
        m_childEntityIdOrder[childIndex].m_sortIndex = childIndex;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::PrepareElementForDestroy()
{
    // destroy child elements, this is complicated by the fact that the child elements
    // will attempt to remove themselves from the m_childEntityIdOrder list in their DestroyElement method.
    // But, if the entities are not initialized yet the child parent pointer will be null.
    // So the child may or may not remove itself from the list.
    // So make a local copy of the list and iterate on that
    if (AreChildPointersValid())
    {
        auto childElementComponents = m_childElementComponents;
        for (auto child : childElementComponents)
        {
            // destroy the child
            child->DestroyElement();
        }
    }
    else
    {
        auto children = m_childEntityIdOrder;   // need a copy
        for (auto& child : children)
        {
            // destroy the child
            UiElementBus::Event(child.m_entityId, &UiElementBus::Events::DestroyElement);
        }
    }

    // remove this element from parent
    if (m_parent)
    {
        GetParentElementComponent()->RemoveChild(GetEntity());
    }

    // Notify listeners that the element is being destroyed
    UiElementNotificationBus::Event(GetEntityId(), &UiElementNotificationBus::Events::OnUiElementBeingDestroyed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiElementComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    if (classElement.GetVersion() < 2)
    {
        // No need to actually convert anything because the CanvasFileObject takes care of it
        // But it makes sense to bump the version number because m_children is now a container
        // of EntityId rather than Entity*
    }

    // conversion from version 2 to 3:
    //      m_children replaced with m_childEntityIdOrder
    // NOTE: We do not go through here if version is 1 since m_children will be an array of Entity*
    // rather than EntityId. That complex conversion is handled in the recursive function
    // MoveEntityAndDescendantsToListAndReplaceWithEntityId
    if (classElement.GetVersion() == 2)
    {
        // Version 3 added the persistent member m_childEntityIdOrder with replaces m_children
        // Find the "Children" element that we will be replacing.
        int childrenIndex = classElement.FindElement(AZ_CRC_CE("Children"));
        if (childrenIndex != -1)
        {
            AZ::SerializeContext::DataElementNode& childrenElementNode = classElement.GetSubElement(childrenIndex);

            // add the new "ChildEntityIdOrder" element, this is a container
            int childOrderIndex = classElement.AddElement<ChildEntityIdOrderArray>(context, "ChildEntityIdOrder");
            AZ::SerializeContext::DataElementNode& childOrderElementNode = classElement.GetSubElement(childOrderIndex);

            int numChildren = childrenElementNode.GetNumSubElements();

            // for each EntityId in the Children container create a ChildEntityIdOrderEntry in the ChildEntityIdOrder container
            for (int childIndex = 0; childIndex < numChildren; ++childIndex)
            {
                AZ::SerializeContext::DataElementNode& childElementNode = childrenElementNode.GetSubElement(childIndex);

                // add the entry in the container (or type ChildEntityIdOrderEntry which is a struct of EntityId and u64)
                int childOrderEntryIndex = childOrderElementNode.AddElement<ChildEntityIdOrderEntry>(context, "element");
                AZ::SerializeContext::DataElementNode& childOrderEntryElementNode = childOrderElementNode.GetSubElement(childOrderEntryIndex);

                // copy the EntityId node from the Children container and change its name
                int childOrderEntryEntityIdIndex = childOrderEntryElementNode.AddElement(childElementNode);
                AZ::SerializeContext::DataElementNode& childOrderEntryEntityIdElementNode = childOrderEntryElementNode.GetSubElement(childOrderEntryEntityIdIndex);
                childOrderEntryEntityIdElementNode.SetName("ChildEntityId");

                // add the the sort index - which is just the position in the container when we are converting old data.
                AZ::u64 sortIndex = childIndex;
                childOrderEntryElementNode.AddElementWithData(context, "SortIndex", sortIndex);
            }

            // remove the old m_children persistent member
            classElement.RemoveElementByName(AZ_CRC_CE("Children"));
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiElementComponent::DestroyElementEntity(AZ::EntityId entityId)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    AzFramework::EntityIdContextQueryBus::EventResult(contextId, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

    UiEntityContextRequestBus::Event(contextId, &UiEntityContextRequestBus::Events::DestroyUiEntity, entityId);
}
