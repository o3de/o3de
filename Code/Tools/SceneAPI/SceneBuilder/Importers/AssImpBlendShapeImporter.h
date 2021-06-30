/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/FbxSceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class AssImpBlendShapeImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpBlendShapeImporter, "{B0F7174B-9863-4C03-BFB2-83BF29B1A2DD}", SceneCore::LoadingComponent);

                AssImpBlendShapeImporter();
                ~AssImpBlendShapeImporter() override = default;
                
                static void Reflect(ReflectContext* context);
                
                Events::ProcessingResult ImportBlendShapes(AssImpSceneNodeAppendedContext& context);
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
