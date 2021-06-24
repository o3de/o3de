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
            class AssImpTransformImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpTransformImporter, "{A7494C53-5822-40EF-9B60-B1FF09FBFA59}", SceneCore::LoadingComponent);

                AssImpTransformImporter();
                ~AssImpTransformImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportTransform(AssImpSceneNodeAppendedContext& context);
                static const char* s_transformNodeName;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
