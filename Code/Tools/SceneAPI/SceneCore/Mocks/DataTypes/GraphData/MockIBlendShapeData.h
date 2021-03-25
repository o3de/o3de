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

#include <gmock/gmock.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>

namespace AZ::SceneAPI::DataTypes
{
    class MockIBlendShapeData
        : public IBlendShapeData
    {
    public:
        MOCK_CONST_METHOD0(GetUsedControlPointCount, size_t());
        MOCK_CONST_METHOD1(GetControlPointIndex, int(int));
        MOCK_CONST_METHOD1(GetUsedPointIndexForControlPoint, int(int));
        MOCK_CONST_METHOD0(GetVertexCount, unsigned int());
        MOCK_CONST_METHOD0(GetFaceCount, unsigned int());
        MOCK_CONST_METHOD1(GetPosition, const AZ::Vector3&(unsigned int index));
        MOCK_CONST_METHOD1(GetNormal, const AZ::Vector3&(unsigned int index));
        MOCK_CONST_METHOD2(GetFaceVertexIndex, unsigned int(unsigned int face, unsigned int vertexIndex));
    };
} // namespace AZ::SceneAPI::DataTypes
