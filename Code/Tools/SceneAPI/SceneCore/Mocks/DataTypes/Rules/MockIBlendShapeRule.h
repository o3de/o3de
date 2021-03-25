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

#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>

namespace AZ::SceneAPI::DataTypes { class IMockSceneNodeSelectionList; }

namespace AZ::SceneAPI::DataTypes
{
    class MockIBlendShapeRule
        : public IBlendShapeRule
    {
    public:
        ISceneNodeSelectionList& GetSceneNodeSelectionList() override
        {
            return const_cast<ISceneNodeSelectionList&>(static_cast<const MockIBlendShapeRule*>(this)->GetSceneNodeSelectionList());
        }

        MOCK_CONST_METHOD0(GetSceneNodeSelectionList, const ISceneNodeSelectionList&());
    };
} // namespace AZ::SceneAPI::DataTypes
