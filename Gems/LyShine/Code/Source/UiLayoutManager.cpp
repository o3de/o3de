/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiLayoutManager.h"

#include <LyShine/Bus/UiLayoutBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutControllerBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutManager::UiLayoutManager(AZ::EntityId canvasEntityId)
{
    UiLayoutManagerBus::Handler::BusConnect(canvasEntityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutManager::~UiLayoutManager()
{
    UiLayoutManagerBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutManager::MarkToRecomputeLayout(AZ::EntityId entityId)
{
    if (UiLayoutControllerBus::FindFirstHandler(entityId))
    {
        AddToRecomputeLayoutList(entityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutManager::MarkToRecomputeLayoutsAffectedByLayoutCellChange(AZ::EntityId entityId, bool isDefaultLayoutCell)
{
    AZ::EntityId topParent;
    AZ::EntityId parent;
    UiElementBus::EventResult(parent, entityId, &UiElementBus::Events::GetParentEntityId);
    while (parent.IsValid())
    {
        bool usesLayoutCells = false;
        UiLayoutBus::EventResult(usesLayoutCells, parent, &UiLayoutBus::Events::IsUsingLayoutCellsToCalculateLayout);
        if (usesLayoutCells && isDefaultLayoutCell)
        {
            bool ignoreDefaultLayoutCells = true;
            UiLayoutBus::EventResult(ignoreDefaultLayoutCells, parent, &UiLayoutBus::Events::GetIgnoreDefaultLayoutCells);
            usesLayoutCells = !ignoreDefaultLayoutCells;
        }

        if (usesLayoutCells)
        {
            topParent = parent;
            parent.SetInvalid();
            UiElementBus::EventResult(parent, topParent, &UiElementBus::Events::GetParentEntityId);
        }
        else
        {
            break;
        }
    }

    if (topParent.IsValid())
    {
        AddToRecomputeLayoutList(topParent);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutManager::UnmarkAllLayouts()
{
    m_elementsToRecomputeLayout.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutManager::RecomputeMarkedLayouts()
{
    for (auto element : m_elementsToRecomputeLayout)
    {
        ComputeLayoutForElementAndDescendants(element);
    }

    UnmarkAllLayouts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutManager::ComputeLayoutForElementAndDescendants(AZ::EntityId entityId)
{
    // Get a list of layout children
    auto FindLayoutChildren = [](const AZ::Entity* entity)
        {
            return (UiLayoutControllerBus::FindFirstHandler(entity->GetId())) ? true : false;
        };

    LyShine::EntityArray layoutChildren;
    UiElementBus::Event(entityId, &UiElementBus::Events::FindDescendantElements, FindLayoutChildren, layoutChildren);

    UiLayoutControllerBus::Event(entityId, &UiLayoutControllerBus::Events::ApplyLayoutWidth);
    for (auto layoutChild : layoutChildren)
    {
        UiLayoutControllerBus::Event(layoutChild->GetId(), &UiLayoutControllerBus::Events::ApplyLayoutWidth);
    }

    UiLayoutControllerBus::Event(entityId, &UiLayoutControllerBus::Events::ApplyLayoutHeight);
    for (auto layoutChild : layoutChildren)
    {
        UiLayoutControllerBus::Event(layoutChild->GetId(), &UiLayoutControllerBus::Events::ApplyLayoutHeight);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutManager::AddToRecomputeLayoutList(AZ::EntityId entityId)
{
    // Check if element is already in the list
    auto iter = AZStd::find(m_elementsToRecomputeLayout.begin(), m_elementsToRecomputeLayout.end(), entityId);
    if (iter == m_elementsToRecomputeLayout.end())
    {
        // Check if element's parent is already in the list
        for (iter = m_elementsToRecomputeLayout.begin(); iter != m_elementsToRecomputeLayout.end(); ++iter)
        {
            if (IsParentOfElement(*iter, entityId))
            {
                // Don't need to add this element
                return;
            }
        }

        // Remove element's children from the list
        LyShine::EntityArray descendants;
        UiElementBus::Event(
            entityId,
            &UiElementBus::Events::FindDescendantElements,
            []([[maybe_unused]] const AZ::Entity* entity)
            {
                return true;
            },
            descendants);

        m_elementsToRecomputeLayout.remove_if(
            [descendants](const AZ::EntityId& e)
            {
                for (auto descendant : descendants)
                {
                    if (descendant->GetId() == e)
                    {
                        return true;
                    }
                }
                return false;
            }
            );

        // Add element to list
        m_elementsToRecomputeLayout.push_back(entityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutManager::IsParentOfElement(AZ::EntityId checkParentEntity, AZ::EntityId checkChildEntity)
{
    AZ::EntityId parent;
    UiElementBus::EventResult(parent, checkChildEntity, &UiElementBus::Events::GetParentEntityId);
    while (parent.IsValid())
    {
        if (parent == checkParentEntity)
        {
            return true;
        }

        AZ::EntityId newParent = parent;
        parent.SetInvalid();
        UiElementBus::EventResult(parent, newParent, &UiElementBus::Events::GetParentEntityId);
    }

    return false;
}

