/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Static/TestImpactBuildTargetDescriptor.h>

namespace TestImpact
{
    BuildTargetDescriptor::BuildTargetDescriptor(BuildMetaData&& buildMetaData, TargetSources&& sources)
        : m_buildMetaData(AZStd::move(buildMetaData))
        , m_sources(AZStd::move(sources))
    {
    }
} // namespace TestImpact
