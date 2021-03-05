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

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>

namespace SceneBuilder
{
    class SceneSerializationHandler
        : public AZ::Component
        , public AZ::SceneAPI::Events::SceneSerializationBus::Handler
    {
    public:
        AZ_COMPONENT(SceneSerializationHandler, "{5917845E-2A6A-4C6C-BD02-E9CECC8D4E13}", AZ::Component);

        SceneSerializationHandler() = default;
        ~SceneSerializationHandler() override = default;

        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> LoadScene(
            const AZStd::string& sceneFilePath, AZ::Uuid sceneSourceGuid) override;
    };
} // namespace SceneBuilder