#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            class ManifestMetaInfoHandler : public Events::ManifestMetaInfoBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR_DECL

                ManifestMetaInfoHandler();
                ~ManifestMetaInfoHandler() override;

                void GetAvailableModifiers(ModifiersList& modifiers, const Containers::Scene& scene, const DataTypes::IManifestObject& target) override;
            };
        } // SceneData
    } // SceneAPI
} // AZ
