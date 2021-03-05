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
    class FocusOnEntityEffect
    {
    public:
        AZ_RTTI(FocusOnEntityEffect, "{583D7B85-6088-4C44-A254-70B04F294BB4}");
        AZ_CLASS_ALLOCATOR(FocusOnEntityEffect, AZ::SystemAllocator, 0);

        FocusOnEntityEffect() = default;
        virtual ~FocusOnEntityEffect() = default;

        virtual AZ::EntityId GetFocusTarget() const = 0;
    };
}
