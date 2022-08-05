/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactNativeTestTargetMeta.h>
#include <Target/Common/TestImpactTestTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Build target specialization for native test targets (build targets containing test code and no production code).
    class NativeTestTarget 
    : public TestTarget
    {
    public:
        NativeTestTarget(TargetDescriptor&& descriptor, NativeTestTargetMeta&& testMetaData);

        //! Returns the launcher custom arguments.
        const AZStd::string& GetCustomArgs() const;
        
        //! Returns the test target launch method.
        LaunchMethod GetLaunchMethod() const;

    private:
        NativeTargetLaunchMeta m_launchMeta;
    };
} // namespace TestImpact
