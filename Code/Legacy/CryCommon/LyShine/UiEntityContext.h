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
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <LyShine/Bus/UiEntityContextBus.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    class EntityContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UI Entity Context stores the prefab asset for the root slice of a UI canvas
//! So all of the UI element entities in a canvas are owned indirectly by the context and managed
//! by the entity context.
class UiEntityContext
    : public AzFramework::EntityContext
    , public UiEntityContextRequestBus::Handler
{
public: // member functions

    //! Initialize the entity context and instantiate the root slice
    virtual void InitUiContext() = 0;

    //! Destroy the Entity Context
    virtual void DestroyUiContext() = 0;

    //! Saves the context's slice root to the specified buffer. If necessary
    //! entities undergo conversion for game: editor -> game components.
    //! \return true if successfully saved. Failure is only possible if serialization data is corrupt.
    virtual bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) = 0;

    //! Saves the given canvas entity to the specified buffer. If necessary
    //! the entity undergoes conversion for game: editor -> game components.
    //! \return true if successfully saved. Failure is only possible if serialization data is corrupt.
    //! This is needed because the canvas entity is not part of the root slice. It is here in the entity
    //! context because that allows us to get to the ToolsFramework functionality.
    virtual bool SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) = 0;
};
