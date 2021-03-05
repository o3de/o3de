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
