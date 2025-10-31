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

        //! Copy and move constructors/assignment operators.
        NativeTestRunJobData(const NativeTestRunJobData& other) = default;
        NativeTestRunJobData(NativeTestRunJobData&& other) = default;
        NativeTestRunJobData& operator=(const NativeTestRunJobData& other) = default;
        NativeTestRunJobData& operator=(NativeTestRunJobData&& other) = default;

        //! Returns the launch method used by this test target.
        LaunchMethod GetLaunchMethod() const;

    private:
        LaunchMethod m_launchMethod = LaunchMethod::TestRunner;
    };

    template<typename Parent>
    LaunchMethod NativeTestRunJobData<Parent>::GetLaunchMethod() const
    {
        return m_launchMethod;
    }
} // namespace TestImpact
