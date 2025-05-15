/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Aabb.h>
#include <SceneAPI/SDKWrapper/NodeWrapper.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SceneCore/Import/SceneImportSettings.h>

namespace AZ
{
    namespace SDKScene
    {
        class SceneWrapperBase
        {
        public:
            AZ_RTTI(SceneWrapperBase, "{703CD344-2C75-4F30-8CE2-6BDEF2511AFD}");
            virtual ~SceneWrapperBase() = default;

            virtual bool LoadSceneFromFile(const char* fileName, const SceneAPI::SceneImportSettings& importSettings = {});
            virtual bool LoadSceneFromFile(const AZStd::string& fileName, const SceneAPI::SceneImportSettings& importSettings = {});

            virtual const std::shared_ptr<SDKNode::NodeWrapper> GetRootNode() const;
            virtual std::shared_ptr<SDKNode::NodeWrapper> GetRootNode();

            virtual void Clear();

            enum class AxisVector
            {
                X = 0,
                Y = 1,
                Z = 2,
                Unknown
            };

            virtual AZStd::pair<AxisVector, int32_t> GetUpVectorAndSign() const;
            virtual AZStd::pair<AxisVector, int32_t> GetFrontVectorAndSign() const;
            virtual AZStd::pair<AxisVector, int32_t> GetRightVectorAndSign() const;
            // some SceneAPI might force the usage of custom root transformation
            // O3DE shall not try to reorient scene in such cases
            virtual AZStd::optional<SceneAPI::DataTypes::MatrixType> UseForcedRootTransform() const;
            virtual float GetUnitSizeInMeters() const;
            virtual AZ::Aabb GetAABB() const;
            virtual uint32_t GetVerticesCount() const;

            static const char* s_defaultSceneName;
        };

        class SceneTypeConverter
        {
        public:
            static SceneAPI::DataTypes::MatrixType ToTransform(const AZ::Matrix4x4& matrix);
        };

    } // namespace SDKScene
} // namespace AZ
