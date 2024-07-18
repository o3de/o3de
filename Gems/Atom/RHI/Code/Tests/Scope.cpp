/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/Scope.h>

namespace UnitTest
{
    using namespace AZ;

    void Scope::InitInternal()
    {
        AZ_Assert(IsInitialized() == false, "Is initialized!");
    }

    void Scope::ActivateInternal()
    {
        AZ_Assert(IsActive() == false, "Is Active");
    }

    void Scope::CompileInternal()
    {
        for (const AZ::RHI::ScopeAttachment* scopeAttachment : GetAttachments())
        {
            ValidateBinding(scopeAttachment);
        }
    }

    void Scope::DeactivateInternal()
    {
        AZ_Assert(IsActive(), "Is not active");
    }

    void Scope::ShutdownInternal()
    {
        AZ_Assert(IsInitialized(), "Is not initialized");
    }

    void Scope::ValidateBinding(const AZ::RHI::ScopeAttachment* scopeAttachment)
    {
        ASSERT_TRUE(&scopeAttachment->GetScope() == this);

        const AZ::RHI::FrameAttachment& attachment = scopeAttachment->GetFrameAttachment();

        bool found = false;
        for (const AZ::RHI::ScopeAttachment* search = attachment.GetFirstScopeAttachment(GetDeviceIndex()); search;
             search = search->GetNext())
        {
            if (search == scopeAttachment)
            {
                found = true;
                break;
            }
        }

        ASSERT_TRUE(found);
    }
}
