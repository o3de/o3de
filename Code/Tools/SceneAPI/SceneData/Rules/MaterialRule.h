/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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
