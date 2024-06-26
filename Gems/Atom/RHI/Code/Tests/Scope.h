/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

    private:
        //////////////////////////////////////////////////////////////////////////
        // RHI::Scope
        void InitInternal() override;
        void ActivateInternal() override;
        void CompileInternal() override;
        void DeactivateInternal() override;
        void ShutdownInternal() override;

        void ValidateBinding(const AZ::RHI::ScopeAttachment* scopeAttachment);
        //////////////////////////////////////////////////////////////////////////
    };
}
