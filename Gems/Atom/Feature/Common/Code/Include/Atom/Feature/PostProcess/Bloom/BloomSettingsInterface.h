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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>

namespace AZ
{
    namespace Render
    {
        class BloomSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::BloomSettingsInterface, "{1FCEEB59-7ED5-4FB7-9884-BC7865A152C5}");

            // Auto-gen virtual getter and setter functions...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
