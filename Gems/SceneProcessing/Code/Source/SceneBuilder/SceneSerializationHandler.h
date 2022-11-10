/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            const AZStd::string& sceneFilePath,
            AZ::Uuid sceneSourceGuid,
            const AZStd::string& watchFolder) override;
    };
} // namespace SceneBuilder
