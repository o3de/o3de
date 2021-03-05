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

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
    

    namespace RHI
    {
        class PlatformLimits;
        struct RHISystemDescriptor final
        {
            AZ_TYPE_INFO(RHISystemDescriptor, "{A506DA28-856C-483A-938D-73471D2C5A5B}");
            static void Reflect(AZ::ReflectContext* context);

            //! The set of globally declared draw list tags, which will be registered with the registry at startup.
            AZStd::vector<AZ::Name> m_drawListTags;

            const RHI::PlatformLimits* m_platformLimits = nullptr;
        };
    } // namespace RHI
} // namespace AZ
