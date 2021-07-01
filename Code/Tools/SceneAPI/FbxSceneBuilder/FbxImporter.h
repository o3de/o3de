/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/SDKWrapper/SceneWrapper.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace FbxSceneBuilder
        {
            class FbxImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxImporter, "{D5EE21B6-8B73-45BF-B711-31346E0BEDB3}", SceneCore::LoadingComponent);

                FbxImporter();
                ~FbxImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportProcessing(Events::ImportEventContext& context);

            protected:
                bool ConvertFbxScene(Containers::Scene& scene) const;
                void SanitizeNodeName(AZStd::string& nodeName) const;

                AZStd::unique_ptr<SDKScene::SceneWrapperBase> m_sceneWrapper;
                AZStd::shared_ptr<FbxSceneSystem> m_sceneSystem;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
