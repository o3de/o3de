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
            class IScriptProcessorRule
                : public IRule
            {
            public:
                AZ_RTTI(IScriptProcessorRule, "{7D595C6C-55BD-44AC-84F9-7E3C74713D7D}", IRule);

                virtual ~IScriptProcessorRule() override = default;

                virtual const AZStd::string& GetScriptFilename() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
