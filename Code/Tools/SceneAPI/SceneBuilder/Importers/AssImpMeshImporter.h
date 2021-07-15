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
            class AssImpMeshImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpMeshImporter, "{41611339-1D32-474A-A6A4-25CE4430AAFB}", SceneCore::LoadingComponent);

                AssImpMeshImporter();
                ~AssImpMeshImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportMesh(AssImpNodeEncounteredContext& context);
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
