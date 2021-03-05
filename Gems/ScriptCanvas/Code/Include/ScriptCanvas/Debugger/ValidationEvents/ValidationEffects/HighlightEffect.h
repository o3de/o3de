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

#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvas
{
    class HighlightEntityEffect
    {
    public:
        AZ_RTTI(HighlightEntityEffect, "{20EA6019-5F3C-43C5-88AF-6F4BF840B2D2}");
        AZ_CLASS_ALLOCATOR(HighlightEntityEffect, AZ::SystemAllocator, 0);
        
        HighlightEntityEffect() = default;
        virtual ~HighlightEntityEffect() = default;
        
        virtual AZ::EntityId GetHighlightTarget() const = 0;
    };
    
    class HighlightVariableEffect
    {
    public:
        AZ_RTTI(HighlightVariableEffect, "{0E684546-FEAD-486C-A166-5C4C9DCAC8D0}");
        AZ_CLASS_ALLOCATOR(HighlightVariableEffect, AZ::SystemAllocator, 0);
        
        HighlightVariableEffect() = default;
        virtual ~HighlightVariableEffect() = default;
        
        virtual VariableId GetHighlightVariableId() const = 0;
    };
}
