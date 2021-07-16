/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
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
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
