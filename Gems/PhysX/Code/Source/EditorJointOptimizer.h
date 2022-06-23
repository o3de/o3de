/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <NumericalMethods/Optimization.h>
#include <NumericalMethods/DoublePrecisionMath/Quaternion.h>

namespace PhysX
{
    struct D6JointLimitConfiguration;
} // namespace PhysX

namespace PhysX::JointLimitOptimizer
{
    struct SwingValues
    {
        SwingValues(const NumericalMethods::DoublePrecisionMath::Quaternion& quaternion);
        double GetViolation(double tanQuarterSwingLimitY, double tanQuarterSwingLimitZ);

        double m_tanQuarterSwingY;
        double m_tanQuarterSwingZ;
    };

    class D6JointLimitFitter
        : public NumericalMethods::Optimization::Function
    {
    public:
        D6JointLimitFitter() = default;
        virtual ~D6JointLimitFitter();

        void SetExampleRotations(const AZStd::vector<AZ::Quaternion>& exampleRotations);
        void SetChildLocalRotation(const AZ::Quaternion& childLocalRotation);
        void SetInitialGuess(const AZ::Quaternion& parentLocalRotation, float swingYRadians, float swingZRadians);
        D6JointLimitConfiguration GetFit(const AZ::Quaternion& childLocalRotation);
        AZ::Outcome<double, NumericalMethods::Optimization::FunctionOutcome> GetObjective(
            const AZStd::vector<double>& x, bool debug = false) const;

    private:
        AZ::u32 GetDimension() const override;
        AZ::Outcome<double, NumericalMethods::Optimization::FunctionOutcome> ExecuteImpl(const AZStd::vector<double>& x) const override;

        AZStd::vector<NumericalMethods::DoublePrecisionMath::Quaternion> m_exampleRotations;
        NumericalMethods::DoublePrecisionMath::Quaternion m_childLocalRotation;
        AZStd::vector<double> m_initialValue;
    };
} // namespace PhysX::JointLimitOptimizer
