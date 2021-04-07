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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/AssImpImportContexts.h>

struct aiAnimation;
struct aiMesh;
struct aiMeshMorphAnim;

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
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

                static const double s_defaultTimeStepSampleRate;

            protected:
                static const char* s_animationNodeName;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
