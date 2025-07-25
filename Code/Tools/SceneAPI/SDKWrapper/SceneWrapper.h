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

            virtual bool LoadSceneFromFile(const char* fileName, const SceneAPI::SceneImportSettings& importSettings);
            bool LoadSceneFromFile(const AZStd::string& fileName, const SceneAPI::SceneImportSettings& importSettings);

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

            //! Gets the up vector axis and its orientation sign from the scene.
            //! @return A pair containing the axis (X, Y, or Z) and sign (1 or -1) indicating the up direction.
            //! Default implementation returns Z axis with sign 1 since that is default in O3DE.
            virtual AZStd::pair<AxisVector, int32_t> GetUpVectorAndSign() const;

            //! Gets the front vector axis and its orientation sign from the scene.
            //! @return A pair containing the axis (X, Y, or Z) and sign (1 or -1) indicating the front direction.
            //! Default implementation returns Y axis with sign 1 since that is the default in O3DE.
            virtual AZStd::pair<AxisVector, int32_t> GetFrontVectorAndSign() const;

            //! Gets the right vector axis and its orientation sign from the scene.
            //! @return A pair containing the axis (X, Y, or Z) and sign (1 or -1) indicating the right direction.
            //! Default implementation returns X axis with sign 1 since that is the default in O3DE.
            virtual AZStd::pair<AxisVector, int32_t> GetRightVectorAndSign() const;

            //! Determines if the scene wrapper forces a custom root transformation matrix.
            //! Some import libraries like AssImp automatically convert scenes to Y-up by applying a root transform.
            //! When this returns a matrix, O3DE will use it instead of trying to reorient the scene based on axis vectors.
            //! @return Optional matrix containing the forced root transformation, or nullopt if no forced transform is used.
            virtual AZStd::optional<SceneAPI::DataTypes::MatrixType> UseForcedRootTransform() const;

            //! Gets the unit scale factor to convert scene units to meters.
            //! @return Scale factor where 1.0 means the scene is already in meters. Default returns 1.0 - the default for O3DE
            virtual float GetUnitSizeInMeters() const;

            //! Gets the axis-aligned bounding box encompassing the entire scene.
            //! @return AABB containing all geometry in the scene. Default returns null AABB.
            virtual AZ::Aabb GetAABB() const;

            //! Gets the total number of vertices across all meshes in the scene.
            //! @return Total vertex count.
            virtual uint32_t GetVerticesCount() const;

            static const char* s_defaultSceneName;
        };

        //! Utility class for converting between different matrix types used in scene processing.
        //! Provides type conversion functions to bridge between SDK-specific matrices and O3DE SceneAPI types.
        class SceneTypeConverter
        {
        public:
            //! Converts an AZ::Matrix4x4 to a SceneAPI MatrixType.
            //! @param matrix The source matrix to convert.
            //! @return The converted matrix in SceneAPI format.
            static SceneAPI::DataTypes::MatrixType ToTransform(const AZ::Matrix4x4& matrix);
        };

    } // namespace SDKScene
} // namespace AZ
