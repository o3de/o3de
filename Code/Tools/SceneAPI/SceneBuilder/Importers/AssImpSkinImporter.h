/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            class AssImpSkinImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpSkinImporter, "{8FBCA725-C04E-42B7-9669-82DB3BB0901F}", SceneCore::LoadingComponent);

                AssImpSkinImporter();
                ~AssImpSkinImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportSkin(AssImpNodeEncounteredContext& context);
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
