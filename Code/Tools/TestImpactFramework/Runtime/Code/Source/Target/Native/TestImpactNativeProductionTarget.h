/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactNativeProductionTargetDescriptor.h>
#include <Target/native/TestImpactNativeTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Build target specialization for production targets (build targets containing production code and no test code).
    class NativeProductionTarget
        : public NativeTarget
    {
    public:
        using Descriptor = NativeProductionTargetDescriptor;
        NativeProductionTarget(AZStd::unique_ptr<Descriptor> descriptor);

    private:
        AZStd::unique_ptr<Descriptor> m_descriptor;
    };

    template<typename Target>
    inline constexpr bool IsProductionTarget = AZStd::is_same_v<NativeProductionTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;
} // namespace TestImpact
