/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
