/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    namespace RHI
    {
        //! Base for the command list Submit item types.
        //! This holds the submission index which is validated against the range for the command list.
        struct SubmitItem
        {
            //! the submitIndex must be in the range specified in the FrameGraphExecuteContext for the command list
            //! (see CommandList::SubmitRange for more information)
            mutable uint32_t m_submitIndex = 0;
        };
    }
}
