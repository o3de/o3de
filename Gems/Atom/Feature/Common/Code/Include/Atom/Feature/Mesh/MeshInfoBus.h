/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Feature/Mesh/MeshInfo.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <AzCore/EBus/EBus.h>

namespace AZ::Render
{
    //! Ebus to receive scene's notifications
    class MeshInfoNotification : public AZ::EBusTraits
    {
    public:
        // EBus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = RPI::SceneId;

        //////////////////////////////////////////////////////////////////////////

        //! Notifies when a new MeshInfo entry was added
        //! @param meshInfoHandle The MeshInfoIndex (as int32_t handle) of the new MeshInfo Entry
        //! @param meshHandle The MeshHandle for the mesh of the new MeshInfo Entry
        //! @param lodIndex The Index of the ModelLod of the mesh
        //! @param lodMeshIndex The Index of the Mesh in the ModelLod
        virtual void CreateMeshInfoEntry(
            [[maybe_unused]] const MeshInfoHandle meshInfoHandle,
            [[maybe_unused]] ModelDataInstanceInterface* modelData,
            [[maybe_unused]] const size_t lodIndex,
            [[maybe_unused]] const size_t lodMeshIndex){};

        //! Notifies when a MeshInfo entry was released
        //! @param meshInfoHandle The MeshInfoIndex (as int32_t handle) of the new MeshInfo Entry
        virtual void DeleteMeshInfoEntry([[maybe_unused]] const MeshInfoHandle meshInfoHandle){};
    };

    using MeshInfoNotificationBus = AZ::EBus<MeshInfoNotification>;

} // namespace AZ::Render
