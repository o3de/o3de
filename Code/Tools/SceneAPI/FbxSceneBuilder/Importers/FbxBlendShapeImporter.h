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
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxBlendShapeImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxBlendShapeImporter, "{3E733F1B-B4A1-4F6F-B2EE-A1C501830E91}", SceneCore::LoadingComponent);

                FbxBlendShapeImporter();
                ~FbxBlendShapeImporter() override = default;
                
                static void Reflect(ReflectContext* context);
                
                Events::ProcessingResult ImportBlendShapes(SceneNodeAppendedContext& context);
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ