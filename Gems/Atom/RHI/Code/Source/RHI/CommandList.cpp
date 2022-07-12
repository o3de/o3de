/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducer.h>

namespace AZ
{
    namespace RHI
    {
        void CommandList::ValidateSubmitItem([[maybe_unused]] const SubmitItem& submitItem)
        {
            if (m_submitRange.GetCount())
            {
                AZ_Assert((submitItem.m_submitIndex >= m_submitRange.m_startIndex) && (submitItem.m_submitIndex < m_submitRange.m_endIndex),
                    "SubmitItem index %d is not in the valid submission range for this CommandList (%d, %d). "
                    "Call FrameGraphExecuteContext::GetSubmitRange() to retrieve the range when submitting items to the CommandList.",
                    submitItem.m_submitIndex,
                    m_submitRange.m_startIndex,
                    m_submitRange.m_endIndex - 1);

                m_totalSubmits++;
            }
        }

        void CommandList::ValidateTotalSubmits([[maybe_unused]] const ScopeProducer* scopeProducer)
        {
            if (m_submitRange.GetCount())
            {
                AZ_Assert(m_totalSubmits <= m_submitRange.GetCount(),
                    "Incorrect number of submit items detected in scope %s (%d expected, %d submitted). "
                    "Call RHI::FrameGraphInterface::SetEstimatedItemCount to set the expected number of submits. "
                    "The number of submits in BuildCommandListInternal must not exceed this value.",
                    scopeProducer->GetScopeId().GetCStr(),
                    m_submitRange.GetCount(),
                    m_totalSubmits);

                ResetTotalSubmits();
            }
        }
    }
}
