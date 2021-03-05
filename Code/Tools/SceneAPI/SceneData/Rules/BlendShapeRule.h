#pragma once

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
