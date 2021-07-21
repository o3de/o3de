/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Tests/SystemComponentFixture.h>
#include <EMotionFX/Source/MotionSet.h>
#include <AzCore/Debug/TraceMessageBus.h>

namespace EMotionFX
{
    class EntityComponentFixture
        : public SystemComponentFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override;
        void TearDown() override;
    };
}
