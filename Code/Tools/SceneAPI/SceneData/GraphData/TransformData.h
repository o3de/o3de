/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneData
    {
        namespace GraphData
        {
            class SCENE_DATA_CLASS TransformData
                : public AZ::SceneAPI::DataTypes::ITransform
            {
            public:
                AZ_RTTI(TransformData, "{EA86343D-8DB4-4907-8CA8-E6BAB8961914}", AZ::SceneAPI::DataTypes::ITransform);

                SCENE_DATA_API TransformData() = default;
                SCENE_DATA_API explicit TransformData(const SceneAPI::DataTypes::MatrixType& transform);

                SCENE_DATA_API virtual void SetMatrix(const SceneAPI::DataTypes::MatrixType& transform);

                SCENE_DATA_API SceneAPI::DataTypes::MatrixType& GetMatrix() override;
                SCENE_DATA_API const SceneAPI::DataTypes::MatrixType& GetMatrix() const override;

                static void Reflect(ReflectContext* context);

            protected:
                SceneAPI::DataTypes::MatrixType m_transform;
            };
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ
