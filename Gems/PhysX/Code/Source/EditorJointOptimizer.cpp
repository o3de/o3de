/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/MathUtils.h>
#include <EditorJointOptimizer.h>
#include <NumericalMethods/DoublePrecisionMath/Quaternion.h>
#include <NumericalMethods/Optimization.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>

namespace PhysX::JointLimitOptimizer
{
    SwingValues::SwingValues(const NumericalMethods::DoublePrecisionMath::Quaternion& quaternion)
    {
        auto twist = NumericalMethods::DoublePrecisionMath::Quaternion();
        if (std::abs(quaternion.GetX()) > 1e-6)
        {
            twist = NumericalMethods::DoublePrecisionMath::Quaternion(quaternion.GetX(), 0.0, 0.0, quaternion.GetW());
        }
        NumericalMethods::DoublePrecisionMath::Quaternion swing = (quaternion * twist.GetConjugate()).GetNormalized();

        if (swing.GetW() < 0.0)
        {
            swing = -swing;
        }

        m_tanQuarterSwingY = swing.GetY() / (1.0 + swing.GetW());
        m_tanQuarterSwingZ = swing.GetZ() / (1.0 + swing.GetW());
    }

    double SwingValues::GetViolation(double tanQuarterSwingLimitY, double tanQuarterSwingLimitZ)
    {
        double yFactor = m_tanQuarterSwingY / tanQuarterSwingLimitY;
        double zFactor = m_tanQuarterSwingZ / tanQuarterSwingLimitZ;

        double tot = (yFactor * yFactor + zFactor * zFactor - 1.0);

        return AZ::GetMax(0.0, tot);
    }

    D6JointLimitFitter::~D6JointLimitFitter()
    {
        m_exampleRotations.clear();
        m_initialValue.clear();
    }

    void D6JointLimitFitter::SetExampleRotations(const AZStd::vector<AZ::Quaternion>& exampleRotations)
    {
        m_exampleRotations.clear();
        m_exampleRotations.reserve(exampleRotations.size());
        for (const auto& exampleRotation : exampleRotations)
        {
            m_exampleRotations.push_back(NumericalMethods::DoublePrecisionMath::Quaternion(exampleRotation) * m_childLocalRotation);
        }
    }

    void D6JointLimitFitter::SetChildLocalRotation(const AZ::Quaternion& childLocalRotation)
    {
        m_childLocalRotation = NumericalMethods::DoublePrecisionMath::Quaternion(childLocalRotation);
    }

    void D6JointLimitFitter::SetInitialGuess(const AZ::Quaternion& parentLocalRotation, float swingYRadians, float swingZRadians)
    {
        m_initialValue.resize(6);
        m_initialValue[0] = parentLocalRotation.GetX();
        m_initialValue[1] = parentLocalRotation.GetY();
        m_initialValue[2] = parentLocalRotation.GetZ();
        m_initialValue[3] = parentLocalRotation.GetW();
        m_initialValue[4] = swingYRadians;
        m_initialValue[5] = swingZRadians;
    }

    D6JointLimitConfiguration D6JointLimitFitter::GetFit(const AZ::Quaternion& childLocalRotation)
    {
        NumericalMethods::Optimization::SolverResult solverResult = NumericalMethods::Optimization::SolverBFGS(*this, m_initialValue);
        AZStd::vector<double> xValues = solverResult.m_xValues;
        NumericalMethods::DoublePrecisionMath::Quaternion parentLocalRotation =
            NumericalMethods::DoublePrecisionMath::Quaternion(xValues[0], xValues[1], xValues[2], xValues[3]).GetNormalized();
        float swingLimitY = aznumeric_cast<float>(xValues[4]);
        float swingLimitZ = aznumeric_cast<float>(xValues[5]);

        D6JointLimitConfiguration fittedLimit;
        fittedLimit.m_parentLocalRotation = parentLocalRotation.ToSingle();
        fittedLimit.m_childLocalRotation = childLocalRotation;
        // value slightly less than pi to ensure limits are definitely inside allowed ranges
        const float limitMax = AZ::Constants::Pi - 0.01f;
        fittedLimit.m_swingLimitY = AZ::RadToDeg(AZ::GetClamp(swingLimitY, 0.0f, limitMax));
        fittedLimit.m_swingLimitZ = AZ::RadToDeg(AZ::GetClamp(swingLimitZ, 0.0f, limitMax));

        // twist values
        float twistMin = AZ::Constants::Pi;
        float twistMax = -AZ::Constants::Pi;
        NumericalMethods::DoublePrecisionMath::Quaternion parentLocalConjugate = parentLocalRotation.GetConjugate();
        for (const auto& exampleRotation : m_exampleRotations)
        {
            NumericalMethods::DoublePrecisionMath::Quaternion relativeRotation = parentLocalConjugate * exampleRotation;
            if (relativeRotation.GetZ() < 0.0)
            {
                relativeRotation = -relativeRotation;
            }
            float twist = aznumeric_cast<float>(2.0 * atan2(relativeRotation.GetX(), relativeRotation.GetW()));
            twist = AZ::Wrap(twist, -AZ::Constants::Pi, AZ::Constants::Pi);
            twistMin = AZ::GetMin(twistMin, twist);
            twistMax = AZ::GetMax(twistMax, twist);
        }
        fittedLimit.m_twistLimitLower = AZ::RadToDeg(AZ::GetClamp(twistMin, -limitMax, limitMax));
        fittedLimit.m_twistLimitUpper = AZ::RadToDeg(AZ::GetClamp(twistMax, -limitMax, limitMax));

        return fittedLimit;
    }

    AZ::Outcome<double, NumericalMethods::Optimization::FunctionOutcome> D6JointLimitFitter::GetObjective(
        const AZStd::vector<double>& x, bool debug) const
    {
        auto parentLocalConjugate =
            NumericalMethods::DoublePrecisionMath::Quaternion(x[0], x[1], x[2], x[3]).GetNormalized().GetConjugate();
        double swingLimitY = fabs(x[4]);
        double swingLimitZ = fabs(x[5]);
        double clampedLimitY = AZ::GetClamp(swingLimitY, 0.0, aznumeric_cast<double>(AZ::Constants::Pi));
        double clampedLimitZ = AZ::GetClamp(swingLimitZ, 0.0, aznumeric_cast<double>(AZ::Constants::Pi));
        double tanQuarterSwingLimitY = tan(0.25f * clampedLimitY);
        double tanQuarterSwingLimitZ = tan(0.25f * clampedLimitZ);

        // violation term
        double objectiveViolation = 0.0;
        const size_t numPoses = m_exampleRotations.size();
        if (numPoses > 0)
        {
            for (const auto& exampleRotation : m_exampleRotations)
            {
                objectiveViolation +=
                    SwingValues(parentLocalConjugate * exampleRotation).GetViolation(tanQuarterSwingLimitY, tanQuarterSwingLimitZ);
            }
            objectiveViolation /= double(numPoses);
        }

        // volume term
        double objectiveVolume = 0.1 * swingLimitY * swingLimitZ;

        double weightViolation = 1.0;
        double weightVolume = 1.0;
        if (debug)
        {
            AZ_Printf("Joint limit fitter", "limit violation term: %f, volume term: %f", objectiveViolation, objectiveVolume);
        }
        double totalObjective = weightViolation * objectiveViolation + weightVolume * objectiveVolume;

        return AZ::Success(totalObjective);
    }

    AZ::u32 D6JointLimitFitter::GetDimension() const
    {
        return 6;
    }

    AZ::Outcome<double, NumericalMethods::Optimization::FunctionOutcome> D6JointLimitFitter::ExecuteImpl(
        const AZStd::vector<double>& x) const
    {
        return GetObjective(x, false);
    }
} // namespace PhysX::JointLimitOptimizer
