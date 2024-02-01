/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Spawnable/ComponentRequirementsValidator.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/EditorOnlyEntityHandler.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/UiEditorOnlyEntityHandler.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/WorldEditorOnlyEntityHandler.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework::Components
{
    class EditorComponentBase;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class AssetPlatformComponentRemover
        : public PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetPlatformComponentRemover, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::AssetPlatformComponentRemover,
            "{25D9A8A6-908F-4B26-A752-EBAF7DC074F8}", PrefabProcessor);

        ~AssetPlatformComponentRemover() override;

        void Process(PrefabProcessorContext& prefabProcessorContext) override;

        using RemoveComponentBasedOnAssetPlatformResult = AZ::Outcome<void, AZStd::string>;
        RemoveComponentBasedOnAssetPlatformResult RemoveComponentBasedOnAssetPlatform(
            PrefabDocument& prefab,
            AZ::SerializeContext* serializeContext,
            PrefabProcessorContext& prefabProcessorContext);

        static void Reflect(AZ::ReflectContext* context);

     protected:
        AZ::SerializeContext* m_serializeContext{ nullptr };
        EditorOnlyEntityHandler* m_editorOnlyEntityHandler{ nullptr };
        EditorOnlyEntityHandlers m_editorOnlyEntityHandlerCandidates{
            aznew WorldEditorOnlyEntityHandler(),
            aznew UiEditorOnlyEntityHandler() };
        EntityIdSet m_editorOnlyEntityIds;

     private:
       //AZStd::map<AZStd::string, AZStd::set<AZ::Uuid>> m_platformTagRemovedComponents;

    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
