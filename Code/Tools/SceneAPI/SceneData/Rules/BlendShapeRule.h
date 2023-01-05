/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>
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
            class SCENE_DATA_CLASS BlendShapeRule
                : public DataTypes::IBlendShapeRule
            {
            public:
                AZ_RTTI(BlendShapeRule, "{E9D04F75-735B-484B-A6F1-5B91F92B36B4}", DataTypes::IBlendShapeRule);
                AZ_CLASS_ALLOCATOR_DECL

                SCENE_DATA_API ~BlendShapeRule() override = default;
                SCENE_DATA_API SceneNodeSelectionList& GetNodeSelectionList();

                SCENE_DATA_API DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
                SCENE_DATA_API const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

                static void Reflect(ReflectContext* context);

            protected:
               SceneNodeSelectionList m_blendShapes;
            };
        } // SceneData
    } // SceneAPI
} // AZ
