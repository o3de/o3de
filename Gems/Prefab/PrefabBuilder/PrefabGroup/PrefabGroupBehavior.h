/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PrefabGroup/PrefabGroup.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/IO/Path/Path.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <AzCore/JSON/rapidjson.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::SceneAPI::Events
{
    class PreExportEventContext;
}

namespace AZ::SceneAPI::Behaviors
{
    class PrefabGroupBehavior
        : public SceneCore::BehaviorComponent
    {
    public:
        AZ_COMPONENT(PrefabGroupBehavior, "{13DC2819-CAC2-4977-91D7-C870087072AB}", SceneCore::BehaviorComponent);

        ~PrefabGroupBehavior() override = default;

        void Activate() override;
        void Deactivate() override;
        static void Reflect(ReflectContext* context);

    private:
        Events::ProcessingResult OnPrepareForExport(Events::PreExportEventContext& context) const;
        AZStd::unique_ptr<rapidjson::Document> CreateProductAssetData(const SceneData::PrefabGroup* prefabGroup, const AZ::IO::Path& relativePath) const;

        bool WriteOutProductAsset(
            Events::PreExportEventContext& context,
            const SceneData::PrefabGroup* prefabGroup,
            const rapidjson::Document& doc) const;

        struct ExportEventHandler;
        AZStd::shared_ptr<ExportEventHandler> m_exportEventHandler;
    };
}
