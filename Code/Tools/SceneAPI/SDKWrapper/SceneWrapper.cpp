/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Math/Aabb.h>
#include <SceneAPI/SDKWrapper/SceneWrapper.h>

namespace AZ
{
    namespace SDKScene
    {
        const char* SceneWrapperBase::s_defaultSceneName = "myScene";

        bool SceneWrapperBase::LoadSceneFromFile([[maybe_unused]] const char* fileName,
            [[maybe_unused]] const SceneAPI::SceneImportSettings& importSettings)
        {
            return false;
        }
        bool SceneWrapperBase::LoadSceneFromFile([[maybe_unused]] const AZStd::string& fileName,
            const SceneAPI::SceneImportSettings& importSettings)
        {
            return LoadSceneFromFile(fileName.c_str(), importSettings);
        }

        const std::shared_ptr<SDKNode::NodeWrapper> SceneWrapperBase::GetRootNode() const
        {
            return {};
        }
        std::shared_ptr<SDKNode::NodeWrapper> SceneWrapperBase::GetRootNode()
        {
            return {};
        }

        void SceneWrapperBase::Clear()
        {
        }

        AZStd::pair<SceneWrapperBase::AxisVector, int32_t> SceneWrapperBase::GetUpVectorAndSign() const
        {
            return {AxisVector::Z, 1};
        }

        AZStd::pair<SceneWrapperBase::AxisVector, int32_t> SceneWrapperBase::GetFrontVectorAndSign() const
        {
            return { AxisVector::X, 1 };
        }

        AZStd::pair<SceneWrapperBase::AxisVector, int32_t> SceneWrapperBase::GetRightVectorAndSign() const
        {
            return { AxisVector::X, 1 };
        }
        AZStd::optional<SceneAPI::DataTypes::MatrixType> SceneWrapperBase::UseForcedRootTransform() const
        {
            return AZStd::nullopt;
        }

        float SceneWrapperBase::GetUnitSizeInMeters() const
        {
            return 1.0f;
        }

        AZ::Aabb SceneWrapperBase::GetAABB() const
        {
            return AZ::Aabb::CreateNull();
        }

        uint32_t SceneWrapperBase::GetVerticesCount() const
        {
            return 0u;
        }


        SceneAPI::DataTypes::MatrixType SceneTypeConverter::ToTransform(const AZ::Matrix4x4& matrix)
        {
            SceneAPI::DataTypes::MatrixType transform;
            for (int row = 0; row < 3; ++row)
            {
                for (int column = 0; column < 4; ++column)
                {
                    transform.SetElement(row, column, matrix.GetElement(row, column));
                }
            }

            return transform;
        }


    } //namespace SDKScene
}// namespace AZ
