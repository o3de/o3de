/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

namespace UnitTest
{
    class MeshVertexColorDataStub
        : public AZ::SceneAPI::DataTypes::IMeshVertexColorData
    {
    public:
        const AZ::Name& GetCustomName() const override
        {
            return m_name;
        }

        size_t GetCount() const override
        {
            return m_colors.size();
        }

        const AZ::SceneAPI::DataTypes::Color& GetColor(size_t index) const override
        {
            return m_colors[index];
        }

        AZ::Name m_name;
        AZStd::vector<AZ::SceneAPI::DataTypes::Color> m_colors;
    };
} // namespace UnitTest
