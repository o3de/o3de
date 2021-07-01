/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneData
    {
        namespace GraphData
        {
            class SCENE_DATA_CLASS BoneData
                : public AZ::SceneAPI::DataTypes::IBoneData
            {
            public:
                AZ_RTTI(BoneData, "{EDFB7CDB-DA39-41F1-800D-1E10421849E5}", AZ::SceneAPI::DataTypes::IBoneData);

                SCENE_DATA_API void SetWorldTransform(const SceneAPI::DataTypes::MatrixType& transform);
                SCENE_DATA_API const SceneAPI::DataTypes::MatrixType& GetWorldTransform() const override;

                static void Reflect(ReflectContext* context);

            protected:
                SceneAPI::DataTypes::MatrixType m_worldTransform;
            };
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ
