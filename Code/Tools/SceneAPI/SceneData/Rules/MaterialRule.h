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
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>

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
            class MaterialRule
                : public DataTypes::IMaterialRule
            {
            public:
                AZ_RTTI(MaterialRule, "{35620013-A27C-4F6D-87BF-72F11688ACAD}", DataTypes::IMaterialRule);
                AZ_CLASS_ALLOCATOR_DECL

                MaterialRule();
                ~MaterialRule() override = default;

                bool RemoveUnusedMaterials() const override;
                bool UpdateMaterials() const override;

                static void Reflect(ReflectContext* context);

            protected:
                bool m_removeMaterials;
                bool m_updateMaterials;
            };
        } // SceneData
    } // SceneAPI
} // AZ
