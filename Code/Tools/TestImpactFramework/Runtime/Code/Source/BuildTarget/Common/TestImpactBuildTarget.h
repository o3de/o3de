/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Common/TestImpactTarget.h>

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/functional.h>

namespace TestImpact
{
    //template<typename TestTarget, typename ProductionTarget>
    //class BuildTarget
    //{
    //    static_assert(
    //        AZStd::is_base_of_v<Target, TestTarget>,
    //        "TestTarget type must derive from Target class in order to interface with the DynamicDependencyMap");
    //
    //    static_assert(
    //        AZStd::is_base_of_v<Target, ProductionTarget>,
    //        "ProductionTarget type must derive from Target class in order to interface with the DynamicDependencyMap");
    //public:
    //    //!
    //    template<typename TargetType>
    //    BuildTarget(const TargetType* target);
    //
    //    //! Returns the generic target pointer for this parent (if any), otherwise nullptr.
    //    const Target* GetTarget() const;
    //
    //    //! Returns the test target pointer for this parent (if any), otherwise nullptr.
    //    const TestTarget* GetTestTarget() const;
    //
    //    //! Returns the production target pointer for this parent (if any), otherwise nullptr.
    //    const ProductionTarget* GetProductionTarget() const;
    //
    //    //!
    //    bool HasTarget() const;
    //
    //    //!
    //    bool HasTestTarget() const;
    //
    //    //!
    //    bool HasProductionTarget() const;
    //
    //    //!
    //    void Visit(const AZStd::function<void()
    //private:
    //    AZStd::variant<AZStd::monostate, const TestTarget*, const ProductionTarget*> m_target;
    //};

    //! Holder for build target types.
    template<typename TestTarget, typename ProductionTarget>
    using BuildTarget = AZStd::variant<const TestTarget*, const ProductionTarget*>;

    //! Optional holder for optional build target types.
    template<typename TestTarget, typename ProductionTarget>
    using OptionalBuildTarget = AZStd::optional<AZStd::variant<const TestTarget*, const ProductionTarget*>>;

} // namespace TestImpact
