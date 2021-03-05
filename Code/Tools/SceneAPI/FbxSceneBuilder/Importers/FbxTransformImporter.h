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

#include <AzCore/Math/Transform.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxTransformImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxTransformImporter, "{354EAAE2-DF31-4E11-BD8A-619419A3EA17}", SceneCore::LoadingComponent);

                FbxTransformImporter();
                ~FbxTransformImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportTransform(SceneNodeAppendedContext& context);

            protected:
                static const char* s_transformNodeName;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ