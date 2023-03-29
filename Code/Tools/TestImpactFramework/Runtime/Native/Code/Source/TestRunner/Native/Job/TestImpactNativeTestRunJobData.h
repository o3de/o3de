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
        NativeTestRunJobData(const NativeTestRunJobData& other);
        NativeTestRunJobData(NativeTestRunJobData&& other);
        NativeTestRunJobData& operator=(const NativeTestRunJobData& other);
        NativeTestRunJobData& operator=(NativeTestRunJobData&& other);

        //! Returns the launch method used by this test target.
        LaunchMethod GetLaunchMethod() const
        {
            return m_launchMethod;
        }

    private:
        LaunchMethod m_launchMethod = LaunchMethod::TestRunner;
    };

    template<typename Parent>
    NativeTestRunJobData<Parent>::NativeTestRunJobData(const NativeTestRunJobData& other)
        : Parent(other)
        , m_launchMethod(other.m_launchMethod)
    {
    }

    template<typename Parent>
    NativeTestRunJobData<Parent>::NativeTestRunJobData(NativeTestRunJobData&& other)
        : Parent(AZStd::move(other))
        , m_launchMethod(AZStd::move(other.m_launchMethod))
    {
    }

    template<typename Parent>
    NativeTestRunJobData<Parent>& NativeTestRunJobData<Parent>::operator=(const NativeTestRunJobData& other)
    {
        m_launchMethod = other.m_launchMethod;
        return *this;
    }

    template<typename Parent>
    NativeTestRunJobData<Parent>& NativeTestRunJobData<Parent>::operator=(NativeTestRunJobData&& other)
    {
        m_launchMethod = AZStd::move(other.m_launchMethod);
        return *this;
    }

    LaunchMethod GetLaunchMethod() const
    {
        return m_launchMethod;
    }
} // namespace TestImpact
