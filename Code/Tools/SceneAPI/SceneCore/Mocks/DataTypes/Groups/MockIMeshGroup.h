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

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIMeshGroup
                : public IMeshGroup
            {
            public:
                AZ_RTTI(MockIMeshGroup, "{AB119B84-E6D8-4CF3-9456-E44B23EDCDFE}", IMeshGroup);

                // IMeshGroup
                MOCK_METHOD0(GetSceneNodeSelectionList,
                    ISceneNodeSelectionList&());
                MOCK_CONST_METHOD0(GetSceneNodeSelectionList,
                    const ISceneNodeSelectionList&());
                MOCK_CONST_METHOD0(GetId, 
                    const Uuid&());
                MOCK_METHOD1(SetName,
                    void(AZStd::string&&));
                MOCK_METHOD1(OverrideId,
                    void(const Uuid&));

                // IGroup
                MOCK_CONST_METHOD0(GetName, 
                    const AZStd::string&());

                MOCK_METHOD0(GetRuleContainer, Containers::RuleContainer&());
                MOCK_CONST_METHOD0(GetRuleContainerConst, const Containers::RuleContainer&());
            };
        }  // namespace DataTypes
    }  // namespace SceneAPI
}  // namespace AZ
