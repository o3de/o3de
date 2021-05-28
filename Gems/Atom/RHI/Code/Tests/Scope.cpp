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

    void Scope::CompileInternal([[maybe_unused]] AZ::RHI::Device& device)
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
        for (
            const AZ::RHI::ScopeAttachment* search = attachment.GetFirstScopeAttachment();
            search;
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
