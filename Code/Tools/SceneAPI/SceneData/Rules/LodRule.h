#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
            class LodRule
                : public DataTypes::ILodRule
            {
            public:
                AZ_RTTI(LodRule, "{6E796AC8-1484-4909-860A-6D3F22A7346F}", DataTypes::ILodRule);
                AZ_CLASS_ALLOCATOR_DECL

                ~LodRule() override = default;

                SceneNodeSelectionList& GetNodeSelectionList(size_t index);

                DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList(size_t index) override;
                const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList(size_t index) const override;
                size_t GetLodCount() const override;

                void AddLod();

                static void Reflect(ReflectContext* context);
                //The engine supports 6 total lods.  1 for the base model then 5 more lods.  
                //The rule only captures lods past level 0 so this is set to 5. 
                static const size_t m_maxLods = 5;
            protected:

                AZStd::fixed_vector<SceneNodeSelectionList, m_maxLods> m_nodeSelectionLists;
            };
        } // SceneData
    } // SceneAPI
} // AZ
