/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Static/TestImpactTestTargetDescriptor.h>

namespace TestImpact
{
    TestTargetDescriptor::TestTargetDescriptor(BuildTargetDescriptor&& buildTarget, TestTargetMeta&& testTargetMeta)
        : BuildTargetDescriptor(AZStd::move(buildTarget))
        , m_testMetaData(AZStd::move(testTargetMeta))
    {
    }
} // namespace TestImpact
