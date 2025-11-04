/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ITagRule
                : public IRule
            {
            public:
                AZ_RTTI(ITagRule, "{B6754CB0-D6FE-4491-97B7-8BEE3DEF81A6}", IRule);

                virtual ~ITagRule() override = default;

                virtual const AZStd::vector<AZStd::string>& GetTags() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
