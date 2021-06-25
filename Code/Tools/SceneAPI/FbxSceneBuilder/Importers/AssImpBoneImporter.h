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
