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
            class AssImpBoneImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpBoneImporter, "{E7A62DE7-B660-4920-BF91-32738175D5A7}", SceneCore::LoadingComponent);

                AssImpBoneImporter();
                ~AssImpBoneImporter() override = default;

                static void Reflect(ReflectContext* context);
                
                Events::ProcessingResult ImportBone(AssImpNodeEncounteredContext& context);
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
