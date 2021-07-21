/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZTOOLSFRAMEWORK_ENTITYSTATECOMMAND_H
#define AZTOOLSFRAMEWORK_ENTITYSTATECOMMAND_H

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Component/EntityId.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

#pragma once

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework
{
    class EntityStateCommandNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void PreRestore() {}
        virtual void PostRestore(AZ::Entity* /*restoredEntity*/) {}
    };

    using EntityStateCommandNotificationBus = AZ::EBus<EntityStateCommandNotifications>;

    // The entity State URSequencePoint stores the state of an entity before and after some change to it.
    // it does so by serializing the entire entity, so its a good "default behavior" that cannot miss any particular change.
    // we can specialize undos (such as the Entity Transform command) to be more specific and narrower in scope
    // but at least an Entity State Command should be able to capture everything in its entirety.
    class EntityStateCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(EntityStateCommand, "{4461579F-9D39-4954-B5D4-0F9388C8D15D}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(EntityStateCommand, AZ::SystemAllocator, 0);

        EntityStateCommand(UndoSystem::URCommandID ID, const char* friendlyName = nullptr);
        virtual ~EntityStateCommand();

        void Undo() override;
        void Redo() override;

        // capture the initial state - this fills the undo with the initial data if captureUndo is true
        // otherwise is captures the final state.
        void Capture(AZ::Entity* pSourceEntity, bool captureUndo);
        AZ::EntityId GetEntityID() const { return m_entityID; }

        bool Changed() const override { return m_undoState != m_redoState; }

    protected:

        void RestoreEntity(const AZ::u8* buffer, AZStd::size_t bufferSizeBytes, const AZ::SliceComponent::EntityRestoreInfo& sliceRestoreInfo) const;

        AZ::EntityId m_entityID;                            ///< The Id of the captured entity.
        AzFramework::EntityContextId m_entityContextId;     ///< The entity context to which the entity belongs (if any).
        AZ::Entity::State m_entityState;                    ///< The entity state at time of capture (active, constructed, etc).
        bool m_isSelected;                                  ///< Whether the entity was selected at time of capture.

        AZ::SliceComponent::EntityRestoreInfo m_undoSliceRestoreInfo;
        AZ::SliceComponent::EntityRestoreInfo m_redoSliceRestoreInfo;

        AZStd::vector<AZ::u8> m_undoState;
        AZStd::vector<AZ::u8> m_redoState;

        // DISABLE COPY
        EntityStateCommand(const EntityStateCommand& other) = delete;
        const EntityStateCommand& operator= (const EntityStateCommand& other) = delete;
    };

    class EntityDeleteCommand
        : public EntityStateCommand
    {
    public:
        AZ_RTTI(EntityDeleteCommand, "{2877DC4C-3F09-4E1A-BE3D-921A021DAB80}", EntityStateCommand);
        AZ_CLASS_ALLOCATOR(EntityDeleteCommand, AZ::SystemAllocator, 0);

        explicit EntityDeleteCommand(UndoSystem::URCommandID ID);
        void Capture(AZ::Entity* pSourceEntity);

        void Undo() override;
        void Redo() override;
    };

    class EntityCreateCommand
        : public EntityStateCommand
    {
    public:
        AZ_RTTI(EntityCreateCommand, "{C1AA9763-9EC8-4F7B-803E-C04EE3DB3DA9}", EntityStateCommand);
        AZ_CLASS_ALLOCATOR(EntityCreateCommand, AZ::SystemAllocator, 0);

        explicit EntityCreateCommand(UndoSystem::URCommandID ID);
        void Capture(AZ::Entity* pSourceEntity);

        void Undo() override;
        void Redo() override;
    };
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_ENTITYSTATECOMMAND_H
