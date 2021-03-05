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

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::ShaderBuilder::AtomShaderConfig
{
    AZ_ENUM(DescriptorSpace,
            Sets,
            Spaces,
            Samplers,
            Textures,
            Buffers);

    struct CapabilitiesConfigFile final
    {
        AZ_RTTI(CapabilitiesConfigFile, "{D3A25140-0F6C-4547-B4E4-0C7B7DE852E6}");

        static void Reflect(AZ::ReflectContext* context);

        //! string key: stringifed version of DescriptorSpace.
        //! int value : -1 for unlimited, or by-specification minimal guaranteed capacity.
        AZStd::unordered_map<AZStd::string, int> m_descriptorCounts;

        //! The max number of spaces supported by the API.
        int m_maxSpaces = -1;
    };
}
