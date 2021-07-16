/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
