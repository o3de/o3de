/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>

struct aiAnimation;
struct aiMesh;
struct aiMeshMorphAnim;

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            class AssImpAnimationImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpAnimationImporter, "{93b3f4e3-6fcd-42b9-a74e-5923f76d25c7}", SceneCore::LoadingComponent);

                AssImpAnimationImporter();
                ~AssImpAnimationImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportAnimation(AssImpSceneNodeAppendedContext& context);
                Events::ProcessingResult ImportBlendShapeAnimation(
                    AssImpSceneNodeAppendedContext& context,
                    const aiAnimation* animation,
                    const aiMeshMorphAnim* meshMorphAnim,
                    const aiMesh* mesh);

                static const double s_defaultTimeStepBetweenFrames;

            protected:
                static const char* s_animationNodeName;
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
