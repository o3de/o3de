/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactNativeTargetDescriptor.h>
#include <Artifact/Static/TestImpactTargetDescriptor.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Target/Common/TestImpactTarget.h>

namespace TestImpact
{
    //! Representation of a generic build target in the repository.
    class NativeTarget
        : public Target
    {
    public:
        NativeTarget(TargetDescriptor&& descriptor, NativeTargetDescriptor&& nativeDescriptor);

        //! Returns the build target's compiled binary name.
        const AZStd::string& GetOutputName() const;
    private:
        NativeTargetDescriptor m_nativeDescriptor;
    };
} // namespace TestImpact
