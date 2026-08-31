/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::Meshlets::Builders
{
    //! SceneAPI behavior that makes MeshletPackRule an ADDABLE modifier in a
    //! model's FBX/Scene Settings ("Add Modifier" dropdown).
    //!
    //! The stock SceneData::ManifestMetaInfoHandler enumerates only the built-in
    //! rules (Lod, Material, Tangents, ...) in its GetAvailableModifiers, so a
    //! gem rule that is merely reflected never appears in the UI. Because
    //! ManifestMetaInfoBus is a Multiple-handler bus, this behavior connects as
    //! an additional handler and appends MeshletPackRule's typeid for IMeshGroup
    //! targets that don't already own one — mirroring the per-rule pattern in
    //! ManifestMetaInfoHandler::GetAvailableModifiers.
    class MeshletPackRuleBehavior
        : public AZ::SceneAPI::SceneCore::BehaviorComponent
        , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
    {
    public:
        AZ_COMPONENT(MeshletPackRuleBehavior,
                     "{3B6F9A41-7C2D-4E58-9A1B-5D8E2F4C7A93}",
                     AZ::SceneAPI::SceneCore::BehaviorComponent);

        ~MeshletPackRuleBehavior() override = default;

        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);

        // ManifestMetaInfoBus::Handler ...
        void GetAvailableModifiers(
            AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
            const AZ::SceneAPI::Containers::Scene& scene,
            const AZ::SceneAPI::DataTypes::IManifestObject& target) override;

        void InitializeObject(
            const AZ::SceneAPI::Containers::Scene& scene,
            AZ::SceneAPI::DataTypes::IManifestObject& target) override;
    };

} // namespace AZ::Meshlets::Builders
