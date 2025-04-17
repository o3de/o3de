/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IMeshVertexUVData;
            class IMeshVertexTangentData;
            class IMeshVertexBitangentData;
        }

        namespace SceneData
        {
            class SCENE_DATA_CLASS TangentsRule
                : public DataTypes::IRule
            {
            public:
                AZ_RTTI(TangentsRule, "{4BD1CE13-D2EB-4CCF-AB21-4877EF69DE7D}", DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(TangentsRule, AZ::SystemAllocator)

                SCENE_DATA_API TangentsRule();
                SCENE_DATA_API ~TangentsRule() override = default;

                SCENE_DATA_API AZ::SceneAPI::DataTypes::TangentGenerationMethod GetGenerationMethod() const;
                SCENE_DATA_API AZ::SceneAPI::DataTypes::MikkTSpaceMethod GetMikkTSpaceMethod() const;

                static void Reflect(ReflectContext* context);

            protected:
                AZ::SceneAPI::DataTypes::TangentGenerationMethod m_generationMethod = AZ::SceneAPI::DataTypes::TangentGenerationMethod::MikkT; /**< Specifies how to handle tangents. Either generate them, or import them. */

                // MikkT specific settings
                AZ::Crc32 GetSpaceMethodVisibility() const;
                AZ::SceneAPI::DataTypes::MikkTSpaceMethod m_tSpaceMethod = AZ::SceneAPI::DataTypes::MikkTSpaceMethod::TSpace;
            };
        } // SceneData
    } // SceneAPI
} // AZ
