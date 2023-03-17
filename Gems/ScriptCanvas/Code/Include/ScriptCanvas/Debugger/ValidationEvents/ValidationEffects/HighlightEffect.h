/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(HighlightEntityEffect, AZ::SystemAllocator);
        
        HighlightEntityEffect() = default;
        virtual ~HighlightEntityEffect() = default;
        
        virtual AZ::EntityId GetHighlightTarget() const = 0;
    };
    
    class HighlightVariableEffect
    {
    public:
        AZ_RTTI(HighlightVariableEffect, "{0E684546-FEAD-486C-A166-5C4C9DCAC8D0}");
        AZ_CLASS_ALLOCATOR(HighlightVariableEffect, AZ::SystemAllocator);
        
        HighlightVariableEffect() = default;
        virtual ~HighlightVariableEffect() = default;
        
        virtual VariableId GetHighlightVariableId() const = 0;
    };
}
