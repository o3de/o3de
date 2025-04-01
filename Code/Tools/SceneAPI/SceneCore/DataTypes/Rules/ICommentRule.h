/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ICommentRule
                : public IRule
            {
            public:
                AZ_RTTI(ICommentRule, "{7CF18D33-C7AB-47F0-8084-A90351154293}", IRule);

                virtual ~ICommentRule() override = default;

                virtual const AZStd::string& GetComment() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
