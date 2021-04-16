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

#include <Atom/Utils/StableDynamicArray.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairRenderObjectInterface
            {
            public:
                AZ_RTTI(AZ::Render::HairRenderObjectInterface, "{21461ADB-A4F7-4021-BA2C-992B3E011D8E}");
            };

            using HairRenderObjectInterfaceHandle = StableDynamicArrayHandle<HairRenderObjectInterface>;
        } // namespace Hair
    } // namespace Render
} // namespace AZ
