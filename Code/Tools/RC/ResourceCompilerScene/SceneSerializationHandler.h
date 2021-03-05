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

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>

namespace AZ
{
    namespace RC
    {
        class SceneSerializationHandler
            : public SceneAPI::SceneCore::SceneSystemComponent
            , public SceneAPI::Events::SceneSerializationBus::Handler
        {
        public:
            AZ_COMPONENT(SceneSerializationHandler, "{944BB08A-FECB-4029-8E7C-810C801357B2}", SceneAPI::SceneCore::SceneSystemComponent);

            SceneSerializationHandler() = default;
            ~SceneSerializationHandler() override = default;

            void Activate() override;
            void Deactivate() override;

            static void Reflect(ReflectContext* context);

            AZStd::shared_ptr<SceneAPI::Containers::Scene> LoadScene(
                const AZStd::string& sceneFilePath, Uuid sceneSourceGuid) override;
        };
    } // namespace RC
} // namespace AZ