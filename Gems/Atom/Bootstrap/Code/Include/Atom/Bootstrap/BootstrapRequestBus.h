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

#include <AzFramework/Scene/SceneSystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render::Bootstrap
{
    class Request
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual AZ::RPI::ScenePtr GetOrCreateAtomSceneFromAzScene(AzFramework::Scene* scene) = 0;
        virtual bool EnsureDefaultRenderPipelineInstalledForScene(AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext) = 0;

    protected:
        ~Request() = default;
    };
    using RequestBus = AZ::EBus<Request>;
} // namespace AZ::Render::Bootstrap
