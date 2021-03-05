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


#include <SceneAPI/FbxSceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class AssImpColorStreamImporter : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpColorStreamImporter, "{071F4764-F3B0-438A-9CB7-19A1248F3B54}", SceneCore::LoadingComponent);

                AssImpColorStreamImporter();
                ~AssImpColorStreamImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportColorStreams(AssImpSceneNodeAppendedContext& context);

            protected:
                static const char* m_defaultNodeName;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
