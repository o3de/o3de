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

#pragma once

#include <SceneAPI/SceneCore/DataTypes/Rules/ICoordinateSystemRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::SceneAPI::SceneData
{
    class SCENE_DATA_CLASS CoordinateSystemRule
        : public DataTypes::ICoordinateSystemRule
    {
    public:
        AZ_RTTI(CoordinateSystemRule, "{53ECEEEA-C489-46DF-9FDB-05251AD960F4}", AZ::SceneAPI::DataTypes::ICoordinateSystemRule);
        AZ_CLASS_ALLOCATOR_DECL

        SCENE_DATA_API CoordinateSystemRule();
        SCENE_DATA_API ~CoordinateSystemRule() override = default;

        SCENE_DATA_API void UpdateCoordinateSystemConverter() override;

        SCENE_DATA_API void SetTargetCoordinateSystem(CoordinateSystem targetCoordinateSystem) override;
        SCENE_DATA_API CoordinateSystem GetTargetCoordinateSystem() const override;

        SCENE_DATA_API const CoordinateSystemConverter& GetCoordinateSystemConverter() const override { return m_coordinateSystemConverter; }

        static void Reflect(AZ::ReflectContext* context);
        SCENE_DATA_API static bool ConvertLegacyCoordinateSystemRule(AZ::SerializeContext& serializeContext,
            AZ::SerializeContext::DataElementNode& classElement);

    protected:
        CoordinateSystemConverter   m_coordinateSystemConverter;
        CoordinateSystem            m_targetCoordinateSystem;
    };
} // namespace AZ::SceneAPI::SceneData
