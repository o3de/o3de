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
    //! Used to represent the extents of the two swing degrees of freedom during optimization of a PhysX D6 joint.
    //! Double precision is used because the BFGS optimization routine may not converge in single precision.
    struct SwingValues
    {
        SwingValues(const NumericalMethods::DoublePrecisionMath::Quaternion& quaternion);
        double GetViolation(double tanQuarterSwingLimitY, double tanQuarterSwingLimitZ);

        double m_tanQuarterSwingY;
        double m_tanQuarterSwingZ;
    };

    //! Used to optimize the orientation and limit values of a PhysX D6 joint, based on sample joint rotations.
    //! The fitter attempts to find an optimal limit cone using an objective function which penalizes violations of the
    //! limit cone by the sample rotations, while simultaneously trying to minimize the size of the cone.
    //! Double precision is used because the BFGS optimization routine may not converge in single precision.
    class D6JointLimitFitter
        : public NumericalMethods::Optimization::Function
    {
    public:
        D6JointLimitFitter() = default;
        virtual ~D6JointLimitFitter();

        void SetLocalRotationSamples(const AZStd::vector<AZ::Quaternion>& localRotationSamples);
        void SetChildLocalRotation(const AZ::Quaternion& childLocalRotation);
        void SetInitialGuess(const AZ::Quaternion& parentLocalRotation, float swingYRadians, float swingZRadians);
        D6JointLimitConfiguration GetFit(const AZ::Quaternion& childLocalRotation);
        AZ::Outcome<double, NumericalMethods::Optimization::FunctionOutcome> GetObjective(
            const AZStd::vector<double>& x, bool debug = false) const;

    private:
        AZ::u32 GetDimension() const override;
        AZ::Outcome<double, NumericalMethods::Optimization::FunctionOutcome> ExecuteImpl(const AZStd::vector<double>& x) const override;

        AZStd::vector<NumericalMethods::DoublePrecisionMath::Quaternion> m_localRotationSamples;
        NumericalMethods::DoublePrecisionMath::Quaternion m_childLocalRotation;
        AZStd::vector<double> m_initialValue;
    };
} // namespace PhysX::JointLimitOptimizer
