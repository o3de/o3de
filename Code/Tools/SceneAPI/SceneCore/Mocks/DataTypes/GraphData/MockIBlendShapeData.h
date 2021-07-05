/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        MOCK_CONST_METHOD1(GetFaceInfo, const Face&(unsigned int index));
        MOCK_CONST_METHOD1(GetPosition, const AZ::Vector3&(unsigned int index));
        MOCK_CONST_METHOD1(GetNormal, const AZ::Vector3&(unsigned int index));
        MOCK_CONST_METHOD2(GetFaceVertexIndex, unsigned int(unsigned int face, unsigned int vertexIndex));
    };
} // namespace AZ::SceneAPI::DataTypes
