/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace SceneData
        {
            class SCENE_DATA_CLASS LodRule
                : public DataTypes::ILodRule
            {
            public:
                AZ_RTTI(LodRule, "{6E796AC8-1484-4909-860A-6D3F22A7346F}", DataTypes::ILodRule);
                AZ_CLASS_ALLOCATOR(LodRule, AZ::SystemAllocator)

                SCENE_DATA_API ~LodRule() override = default;

                SCENE_DATA_API SceneNodeSelectionList& GetNodeSelectionList(size_t index);

                SCENE_DATA_API DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList(size_t index) override;
                SCENE_DATA_API const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList(size_t index) const override;
                SCENE_DATA_API size_t GetLodCount() const override;

                SCENE_DATA_API void AddLod();

                SCENE_DATA_API bool GetAutoGenerateLods() const;

                SCENE_DATA_API bool GetSimplifyPreserveTopology() const;

                SCENE_DATA_API bool GetSimplifyLimitError() const;

                SCENE_DATA_API bool GetSimplifyLockBorder() const;

                SCENE_DATA_API bool GetSimplifySparse() const;

                SCENE_DATA_API bool GetSimplifyPrune() const;

                SCENE_DATA_API float GetSimplifyTargetError() const;

                SCENE_DATA_API float GetSimplifyIndexThreshold() const;

                static void Reflect(ReflectContext* context);
                //The engine supports 6 total lods.  1 for the base model then 5 more lods.
                //The rule only captures lods past level 0 so this is set to 5.
                static const size_t m_maxLods = 5;
            protected:
                bool m_autoGenerateLods = false;
                bool m_simplifyPreserveTopology = false;
                bool m_simplifyLimitError = false;
                bool m_simplifyLockBorder = false;
                bool m_simplifySparse = false;
                bool m_simplifyPrune = false;
                float m_simplifyTargetError = 0.1;
                float m_simplifyIndexThreshold = 0.5f;

                AZStd::fixed_vector<SceneNodeSelectionList, m_maxLods> m_nodeSelectionLists;
            private:
                AZ::Crc32 GetAutoLodsVisibility() const;

                AZ::Crc32 GetErrorLimitVisibility() const;

                AZ::Crc32 GetTopologySettingsVisibility() const;
            };
        } // SceneData
    } // SceneAPI
} // AZ
