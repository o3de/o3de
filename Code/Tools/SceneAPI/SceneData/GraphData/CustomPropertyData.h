/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ICustomPropertyData.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneData::GraphData
    {
        class SCENE_DATA_CLASS CustomPropertyData
            : public AZ::SceneAPI::DataTypes::ICustomPropertyData
        {
        public:
            using PropertyMap = SceneAPI::DataTypes::ICustomPropertyData::PropertyMap;

            AZ_RTTI(CustomPropertyData, "{19BC99F8-E461-4079-B734-E2628B0B1837}", AZ::SceneAPI::DataTypes::ICustomPropertyData);

            SCENE_DATA_API CustomPropertyData() = default;
            SCENE_DATA_API explicit CustomPropertyData(PropertyMap properyMap);
            SCENE_DATA_API virtual void SetPropertyMap(const PropertyMap& properyMap);

            SCENE_DATA_API PropertyMap& GetPropertyMap() override;
            SCENE_DATA_API const PropertyMap& GetPropertyMap() const override;
            SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;

            static void Reflect(ReflectContext* context);

        protected:
            PropertyMap m_propertyMap;
        };
    } // namespace SceneData::GraphData
} // namespace AZ
