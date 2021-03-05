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


#include <AzCore/Memory/PoolAllocator.h>

namespace MCore
{
    class AttributeAllocator
        : public AZ::PoolAllocator
    {
        friend class AZ::AllocatorInstance<AZ::PoolAllocator>;
    public:
        AZ_TYPE_INFO(AttributeAllocator, "{005003CF-87D1-4DAD-A159-59217F67886B}");
        const char* GetName() const override { return TYPEINFO_Name(); }
        const char* GetDescription() const override;
    };
}
