/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactNativeTestTargetDescriptor.h>
#include <Target/Native/TestImpactNativeTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Build target specialization for test targets (build targets containing test code and no production code).
    class NativeTestTarget
        : public NativeTarget
    {
    public:
        using Descriptor = NativeTestTargetDescriptor;

        NativeTestTarget(AZStd::unique_ptr<Descriptor> descriptor);

        //! Returns the test target suite.
        const AZStd::string& GetSuite() const;

        //! Returns the launcher custom arguments.
        const AZStd::string& GetCustomArgs() const;

        //! Returns the test run timeout.
        AZStd::chrono::milliseconds GetTimeout() const;

        //! Returns the test target launch method.
        LaunchMethod GetLaunchMethod() const;

    private:
        AZStd::unique_ptr<Descriptor> m_descriptor;
    };

    template<typename Target>
    inline constexpr bool IsTestTarget = AZStd::is_same_v<NativeTestTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;
} // namespace TestImpact
