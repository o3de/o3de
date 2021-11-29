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
    enum BuildTargetType : AZ::u8
    {
        TestTarget,
        ProductionTarget
    };

    template<typename TestTarget, typename ProductionTarget>
    class BuildTarget
    {
    public:
        //!
        BuildTarget(const TestTarget* testTarget);

        //!
        BuildTarget(const ProductionTarget* productionTarget);
    
        //! Returns the generic target pointer for this parent, otherwise nullptr.
        const Target* GetTarget() const;
    
        //! Returns the test target pointer for this parent (if any), otherwise nullptr.
        AZStd::optional<const TestTarget*> GetTestTarget() const;
    
        //! Returns the production target pointer for this parent (if any), otherwise nullptr.
        AZStd::optional<const ProductionTarget*> GetProductionTarget() const;

        //!
        BuildTargetType GetTargetType() const;
    
        //!
        template<typename Visitor>
        void Visit(const Visitor& visitor) const;

        //!
        BuildTargetType m_type;

    private:
        //!
        template<typename Target>
        static constexpr bool IsProductionTarget =
            AZStd::is_same_v<ProductionTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;

        //!
        template<typename Target>
        static constexpr bool IsTestTarget =
            AZStd::is_same_v<TestTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;
        AZStd::variant<const TestTarget*, const ProductionTarget*> m_target;
    };

    //! Optional holder for optional build target types.
    template<typename TestTarget, typename ProductionTarget>
    using OptionalBuildTarget = AZStd::optional<BuildTarget<TestTarget, ProductionTarget>>;

    template<typename TestTarget, typename ProductionTarget>
    BuildTarget<TestTarget, ProductionTarget>::BuildTarget(const TestTarget* testTarget)
        : m_target(testTarget)
        , m_type(BuildTargetType::TestTarget)
    {
    }

    template<typename TestTarget, typename ProductionTarget>
    BuildTarget<TestTarget, ProductionTarget>::BuildTarget(const ProductionTarget* productionTarget)
        : m_target(productionTarget)
        , m_type(BuildTargetType::ProductionTarget)
    {
    }

    template<typename TestTarget, typename ProductionTarget>
    const Target* BuildTarget<TestTarget, ProductionTarget>::GetTarget() const
    {
        const Target* returnTarget = nullptr;
        Visit(
            [&returnTarget](auto&& target)
            {
                returnTarget = &(*target);
            });

        return returnTarget;
    }

    template<typename TestTarget, typename ProductionTarget>
    AZStd::optional<const TestTarget*> BuildTarget<TestTarget, ProductionTarget>::GetTestTarget() const
    {
        AZStd::optional<const TestTarget*> testTarget;
        Visit(
            [&testTarget](auto&& target)
            {
                if constexpr (IsTestTarget<decltype(target)>)
                {
                    testTarget = target;
                }
            });

        return testTarget;
    }

    template<typename TestTarget, typename ProductionTarget>
    AZStd::optional<const ProductionTarget*> BuildTarget<TestTarget, ProductionTarget>::GetProductionTarget() const
    {
        AZStd::optional<const ProductionTarget*> productionTarget;
        Visit(
            [&productionTarget](auto&& target)
            {
                if constexpr (IsProductionTarget<decltype(target)>)
                {
                    productionTarget = target;
                }
            });

        return productionTarget;
    }

    template<typename TestTarget, typename ProductionTarget>
    BuildTargetType BuildTarget<TestTarget, ProductionTarget>::GetTargetType() const
    {       
        return m_type;
    }

    template<typename TestTarget, typename ProductionTarget>
    template<typename Visitor>
    void BuildTarget<TestTarget, ProductionTarget>::Visit(const Visitor& visitor) const
    {
        AZStd::visit(visitor, m_target);
    }

    //!
    template<typename TestTarget, typename ProductionTarget>
    bool operator==(const BuildTarget<TestTarget, ProductionTarget>& lhs, const BuildTarget<TestTarget, ProductionTarget>& rhs)
    {
        return lhs.GetTarget() == rhs.GetTarget();
    }
} // namespace TestImpact

namespace AZStd
{
    //! Hash function for BuildTarget types for use in maps and sets.
    template<typename TestTarget, typename ProductionTarget>
    struct hash<TestImpact::BuildTarget<TestTarget, ProductionTarget>>
    {
        size_t operator()(const TestImpact::BuildTarget<TestTarget, ProductionTarget>& buildTarget) const noexcept
        {
            return reinterpret_cast<size_t>(buildTarget.GetTarget());
        }
    };
} // namespace AZStd
