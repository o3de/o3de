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
            class AssImpBitangentStreamImporter : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpBitangentStreamImporter, "{49FC818A-956F-43DA-BBAC-73198E0C5A1F}", SceneCore::LoadingComponent);

                AssImpBitangentStreamImporter();
                ~AssImpBitangentStreamImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportBitangentStreams(AssImpSceneNodeAppendedContext& context);

            protected:
                static const char* m_defaultNodeName;
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
