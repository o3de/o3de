/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactNativeTestTargetMeta.h>

namespace TestImpact
{
    template<typename Parent>
    class NativeTestRunJobData
        : public Parent
    {
    public:
        template<typename... AdditionalInfoArgs>
        NativeTestRunJobData(LaunchMethod launchMethod, AdditionalInfoArgs&&... additionalInfo)
            : Parent(std::forward<AdditionalInfoArgs>(additionalInfo)...)
            , m_launchMethod(launchMethod)
        {
        }

        LaunchMethod GetLaunchMethod() const
        {
            return m_launchMethod;
        }

    private:
        LaunchMethod m_launchMethod = LaunchMethod::TestRunner;
    };
} // namespace TestImpact
