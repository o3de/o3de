/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            // Disables UI interaction for the node and children of the node with this rule.
            class IUnmodifiableRule : public IRule
            {
            public:
                AZ_RTTI(IUnmodifiableRule, "{071A7F4A-70A5-4D32-AC9C-1D49AFCB1480}", IRule);

                virtual ~IUnmodifiableRule() override = default;
            };
        } // namespace DataTypes
    } // namespace SceneAPI
} // namespace AZ
