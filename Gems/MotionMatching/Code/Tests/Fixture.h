/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <MotionMatchingSystemComponent.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX::MotionMatching
{
    using Fixture = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        EMotionFX::Integration::SystemComponent,
        MotionMatchingSystemComponent
    >;
}
