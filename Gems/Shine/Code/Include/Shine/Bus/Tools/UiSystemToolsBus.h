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
#include <AzCore/Serialization/ObjectStream.h>

namespace AZ
{
    namespace IO
    {
        class FileIOStream;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus interface for tools to talk to the Shine system
//! It is valid to use this bus from resource compilers or the UI Editor
class UiSystemToolsInterface
    : public AZ::EBusTraits
{
public: // types

    class CanvasAssetHandle
    {
    public:
        virtual ~CanvasAssetHandle() {};
    };

public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    using MutexType = AZStd::recursive_mutex;

    // Public functions

    //! Load a canvas but do not init or activate the entities
    //! The CanvasAssetHandle is an opaque pointer only valid to be passed to the
    //! methods below.
    virtual CanvasAssetHandle* LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc) = 0;

    //! Save a canvas to a stream
    virtual void SaveCanvasToStream(CanvasAssetHandle* canvas, AZ::IO::FileIOStream& stream) = 0;

    //! Get the child entities for a loaded canvas
    virtual AZStd::vector<AZ::Entity*>& GetChildEntities(CanvasAssetHandle* canvas) = 0;

    //! Get the canvas entity for a loaded canvas
    virtual AZ::Entity* GetCanvasEntity(CanvasAssetHandle* canvas) = 0;

    //! Replace the child entities with new ones. The old entities are not deleted.
    //! The client is responsible for that.
    virtual void ReplaceChildEntities(CanvasAssetHandle* canvas, AZStd::vector<AZ::Entity*> newEntities) = 0;

    //! Replace the canvas entity with a new one. The old canvas entity is not deleted.
    //! The client is responsible for that.
    virtual void ReplaceCanvasEntity(CanvasAssetHandle* canvas, AZ::Entity* newCanvasEntity) = 0;
    
    //! Delete the canvas file object and its canvas entity and child entities.
    virtual void DestroyCanvas(CanvasAssetHandle* canvas) = 0;
};

using UiSystemToolsBus = AZ::EBus<UiSystemToolsInterface>;

