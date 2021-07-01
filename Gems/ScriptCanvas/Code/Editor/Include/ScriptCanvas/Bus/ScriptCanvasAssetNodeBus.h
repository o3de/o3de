/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <GraphCanvas/Components/SceneBus.h>

namespace AZ
{
    class Vector2;
}

namespace GraphCanvas
{
    class SceneSliceInstance;
    class SceneSliceReference;
    struct ExposedEndpointInfo;

    //! SubSceneRequests
    //! EBus for forwarding scene request for a node to the a contained sub scene
    class SubSceneRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Retrieves the SceneSlice reference of the sub scene
        virtual SceneSliceReference* GetReference() = 0;
        virtual const SceneSliceReference* GetReferenceConst() const = 0;

        //! Retrieves the SceneSlice instance from the sub scene scene slice reference
        virtual SceneSliceInstance* GetInstance() = 0;
    };

    using SubSceneRequestBus = AZ::EBus<SubSceneRequests>;
}
