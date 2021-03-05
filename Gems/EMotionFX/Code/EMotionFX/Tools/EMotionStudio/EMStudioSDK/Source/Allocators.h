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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace EMStudio
{
    /**
     * System allocator to be used for UI-related objects.
     */
    class UIAllocator
        : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>
    {
    public:
        AZ_TYPE_INFO(UIAllocator, "{98AED295-91AE-4598-B253-90A67FE4DABC}");
        using Base = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>;
        using Descriptor = Base::Descriptor;

        UIAllocator();
    };
}
