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
        NativeTestRunJobData(const NativeTestRunJobData& other)
            : Parent(other)
            , m_launchMethod(other.m_launchMethod)
        {
        }

        NativeTestRunJobData(NativeTestRunJobData&& other)
            : Parent(AZStd::move(other))
            , m_launchMethod(AZStd::move(other.m_launchMethod))
        {
        }

        NativeTestRunJobData& operator=(const NativeTestRunJobData& other)
        {
            m_launchMethod = other.m_launchMethod;
            return *this;
        }

        NativeTestRunJobData& operator=(NativeTestRunJobData&& other)
        {
            m_launchMethod = AZStd::move(other.m_launchMethod);
            return *this;
        }


        LaunchMethod GetLaunchMethod() const
        {
            return m_launchMethod;
        }

    private:
        LaunchMethod m_launchMethod = LaunchMethod::TestRunner;
    };
} // namespace TestImpact
