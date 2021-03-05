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

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/FrameAttachment.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class Scope
        : public AZ::RHI::Scope
    {
    public:
        AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator, 0);

    private:
        //////////////////////////////////////////////////////////////////////////
        // RHI::Scope
        void InitInternal() override;
        void ActivateInternal() override;
        void CompileInternal(AZ::RHI::Device& device) override;
        void DeactivateInternal() override;
        void ShutdownInternal() override;

        void ValidateBinding(const AZ::RHI::ScopeAttachment* scopeAttachment);
        //////////////////////////////////////////////////////////////////////////
    };
}