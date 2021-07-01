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
            class AssImpMaterialImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpMaterialImporter, "{CD936FA9-17B8-40B9-AA3C-5F593BEFFC94}", SceneCore::LoadingComponent);

                AssImpMaterialImporter();
                ~AssImpMaterialImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportMaterials(AssImpSceneNodeAppendedContext& context);

            private:
                AZStd::string ResolveTexturePath(const AZStd::string& sceneFilePath, const AZStd::string& textureFilePath) const;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
