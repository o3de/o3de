/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
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
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
