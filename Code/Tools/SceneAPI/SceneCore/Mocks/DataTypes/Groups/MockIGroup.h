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
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIGroup
                : public IGroup
            {
            public:
                AZ_RTTI(MockIGroup, "{533C7569-78CC-4012-A846-8FD232413F42}", IGroup);

                MOCK_METHOD0(GetRuleContainer, Containers::RuleContainer&());
                MOCK_CONST_METHOD0(GetRuleContainerConst, const Containers::RuleContainer&());
                MOCK_CONST_METHOD0(GetName, AZStd::string&());
                MOCK_CONST_METHOD0(GetId, Uuid&());
            };
        }  // namespace DataTypes
    }  // namespace SceneAPI
}  // namespace AZ
