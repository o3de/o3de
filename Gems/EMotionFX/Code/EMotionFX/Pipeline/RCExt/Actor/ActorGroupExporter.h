/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/optional.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <Integration/System/SystemCommon.h>


namespace EMotionFX
{
    class Actor;

    namespace Pipeline
    {
        struct ActorGroupExportContext;

        class ActorGroupExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(ActorGroupExporter, "{9E21DF50-202B-44CB-A9E7-F429D3B5E5BE}", AZ::SceneAPI::SceneCore::ExportingComponent);

            ActorGroupExporter();

            Actor* GetActor() const;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(ActorGroupExportContext& context);

        private:
            AZ::SceneAPI::Events::ProcessingResult SaveActor(ActorGroupExportContext& context);

            //! Get the mesh asset id to which the actor is linked to by default.
            AZStd::optional<AZ::Data::AssetId> GetMeshAssetId(const ActorGroupExportContext& context) const;
            static AZStd::optional<AZ::SceneAPI::Events::ExportProduct> GetFirstProductByType(
                const ActorGroupExportContext& context, AZ::Data::AssetType type);

            AZStd::shared_ptr<Actor> m_actor;
            AZStd::vector<AZStd::string> m_actorMaterialReferences;
        };
    } // namespace Pipeline
} // namespace EMotionFX
