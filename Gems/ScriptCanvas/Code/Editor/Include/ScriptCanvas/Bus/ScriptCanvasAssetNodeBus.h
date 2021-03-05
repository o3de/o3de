/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
