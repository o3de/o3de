/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiLayoutManagerInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiLayoutManagerInterface() {}

public: // static member data

    //! Mark an element to recompute its layout. This is called when something that affects the layout
    //! has been modified. (ex. layout element size changed, layout element property changed, layout
    //! element child count changed.)
    virtual void MarkToRecomputeLayout(AZ::EntityId entityId) = 0;

    //! Mark the specified element's parent to recompute its layout. The parent uses its child's layout
    //! cell values to calculate its layout, so this is called when something that affects the
    //! child's layout cell values has been modified. (ex. child's layout cell property changed.)
    //! Since a child's layout cell values may affect its parent's layout cell values, the top level parent
    //! is marked
    virtual void MarkToRecomputeLayoutsAffectedByLayoutCellChange(AZ::EntityId entityId, bool isDefaultLayoutCell) = 0;

    //! Unmark all elements from needing to recompute their layouts
    virtual void UnmarkAllLayouts() = 0;

    //! Recompute layouts of marked elements and clear the marked layout list
    virtual void RecomputeMarkedLayouts() = 0;

    //! Compute the layout for the specified element and its descendants
    virtual void ComputeLayoutForElementAndDescendants(AZ::EntityId entityId) = 0;

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiLayoutManagerInterface> UiLayoutManagerBus;
