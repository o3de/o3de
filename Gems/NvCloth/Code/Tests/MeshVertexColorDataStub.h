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
