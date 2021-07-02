/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Tests/UI/AnimGraphUIFixture.h>

namespace EMotionFX
{
    class PreviewMotionFixture :
        public AnimGraphUIFixture
    {
    public:
        void SetUp() override;
        
    public:
        AZStd::string m_motionFileName;
        AZStd::string m_motionName;
    };
} // end namespace EMotionFX
