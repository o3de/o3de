/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <Shine/Bus/UiEntityContextBus.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    class EntityContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UI Entity Context manages all UI element entities in a canvas.
//! Entities are owned by the context via the entity ownership service.
class UiEntityContext
    : public AzFramework::EntityContext
    , public UiEntityContextRequestBus::Handler
{
public: // member functions

    //! Initialize the entity context
    virtual void InitUiContext() = 0;

    //! Destroy the Entity Context
    virtual void DestroyUiContext() = 0;

    //! Saves all entities to the specified buffer. If necessary
    //! entities undergo conversion for game: editor -> game components.
    //! \return true if successfully saved. Failure is only possible if serialization data is corrupt.
    virtual bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) = 0;

    //! Saves the given canvas entity to the specified buffer. If necessary
    //! the entity undergoes conversion for game: editor -> game components.
    //! \return true if successfully saved. Failure is only possible if serialization data is corrupt.
    //! This is needed because the canvas entity is separate from the child entities. It is here in the entity
    //! context because that allows us to get to the ToolsFramework functionality.
    virtual bool SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) = 0;

    //! Load a set of pre-deserialized entities into this context
    //! \param entities The entities to load (ownership is taken)
    //! \param remapIds If true, generate new entity IDs and fix up references
    //! \param idRemapTable Optional map to store the old-to-new entity ID mapping
    virtual bool HandleLoadedEntities(const AZStd::vector<AZ::Entity*>& entities, bool remapIds,
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId>* idRemapTable = nullptr) = 0;

    //! Get all entities managed by this context
    void GetAllEntities(AzFramework::EntityList& entities)
    {
        if (m_entityOwnershipService)
        {
            m_entityOwnershipService->GetAllEntities(entities);
        }
    }
};
