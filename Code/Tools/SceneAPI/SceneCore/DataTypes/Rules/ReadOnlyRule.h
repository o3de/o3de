#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            // Disables UI interaction for the node and children of the node with this rule.
            class ReadOnlyRule : public IRule
            {
            public:
                AZ_RTTI(ReadOnlyRule, "{071A7F4A-70A5-4D32-AC9C-1D49AFCB1480}", IRule);

                virtual ~ReadOnlyRule() override = default;

                bool ModifyTooltip(AZStd::string& tooltip) override
                {
                    tooltip = AZStd::string::format("This group is read only. %.*s", AZ_STRING_ARG(tooltip));
                    return true;
                }
            };
        } // namespace DataTypes
    } // namespace SceneAPI
} // namespace AZ
