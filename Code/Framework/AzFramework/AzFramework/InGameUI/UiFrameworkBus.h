/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus interface for other systems in game to access to the in-game UI system
class UiFrameworkInterface
    : public AZ::EBusTraits
{
public: // types

    using EntityList = AZStd::vector<AZ::Entity*>;
    using EntityIdSet = AZStd::unordered_set<AZ::EntityId>;

public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    using MutexType = AZStd::recursive_mutex;

    // Public functions

    //! Returns true if the given entity has a UiElementComponent, false otherwise.
    virtual bool HasUiElementComponent(AZ::Entity* entity) = 0;

    //! Given an editor-only entity, add it to the given list of editor-only entity IDs.
    //!
    //! The UI system will add all descendants of the editor-only entity to the list of
    //! editor-only entity IDs. Any entity that is editor-only will also have its child
    //! hiearchy treated as editor only.
    virtual void AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities) = 0;

    //! Removes all editor-only entities from the entity hierarchy.
    virtual void HandleEditorOnlyEntities(const EntityList& exportSliceEntities, const EntityIdSet& editorOnlyEntityIds) = 0;
};

using UiFrameworkBus = AZ::EBus<UiFrameworkInterface>;

