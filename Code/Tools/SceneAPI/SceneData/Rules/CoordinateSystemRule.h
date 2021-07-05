/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        SCENE_DATA_API static void Reflect(AZ::ReflectContext* context);
        SCENE_DATA_API static bool ConvertLegacyCoordinateSystemRule(AZ::SerializeContext& serializeContext,
            AZ::SerializeContext::DataElementNode& classElement);

        // advanced coordinate settings
        SCENE_DATA_API bool GetUseAdvancedData() const override;
        SCENE_DATA_API void SetUseAdvancedData(bool useAdvancedData);
        SCENE_DATA_API const AZStd::string& GetOriginNodeName() const override;
        SCENE_DATA_API void SetOriginNodeName(const AZStd::string& originNodeName);
        SCENE_DATA_API const Quaternion& GetRotation() const override;
        SCENE_DATA_API void SetRotation(const Quaternion& rotation);
        SCENE_DATA_API const Vector3& GetTranslation() const override;
        SCENE_DATA_API void SetTranslation(const Vector3& translation);
        SCENE_DATA_API float GetScale() const override;
        SCENE_DATA_API void SetScale(float scale);

    protected:
        AZ::Crc32 GetBasicVisibility() const;
        AZ::Crc32 GetAdvancedVisibility() const;

        CoordinateSystemConverter   m_coordinateSystemConverter;
        CoordinateSystem            m_targetCoordinateSystem;

        // advanced coordinate settings
        bool                        m_useAdvancedData = false;
        AZStd::string               m_originNodeName;
        AZ::Quaternion              m_rotation = AZ::Quaternion::CreateIdentity();
        AZ::Vector3                 m_translation = AZ::Vector3::CreateZero();
        float                       m_scale = 1.0f;
    };
} // namespace AZ::SceneAPI::SceneData
