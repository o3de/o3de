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
        m_localRotationSamples.clear();
        m_initialValue.clear();
    }

    void D6JointLimitFitter::SetLocalRotationSamples(const AZStd::vector<AZ::Quaternion>& localRotationSamples)
    {
        m_localRotationSamples.clear();
        m_localRotationSamples.reserve(localRotationSamples.size());
        for (const auto& localRotationSample : localRotationSamples)
        {
            m_localRotationSamples.push_back(NumericalMethods::DoublePrecisionMath::Quaternion(localRotationSample) * m_childLocalRotation);
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
        const NumericalMethods::Optimization::SolverResult solverResult = NumericalMethods::Optimization::SolverBFGS(*this, m_initialValue);
        const AZStd::vector<double> xValues = solverResult.m_xValues;
        const NumericalMethods::DoublePrecisionMath::Quaternion parentLocalRotation =
            NumericalMethods::DoublePrecisionMath::Quaternion(xValues[0], xValues[1], xValues[2], xValues[3]).GetNormalized();
        const float swingLimitY = aznumeric_cast<float>(xValues[4]);
        const float swingLimitZ = aznumeric_cast<float>(xValues[5]);

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
        for (const auto& localRotationSample : m_localRotationSamples)
        {
            NumericalMethods::DoublePrecisionMath::Quaternion relativeRotation = parentLocalConjugate * localRotationSample;
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
        const auto parentLocalConjugate =
            NumericalMethods::DoublePrecisionMath::Quaternion(x[0], x[1], x[2], x[3]).GetNormalized().GetConjugate();
        const double swingLimitY = fabs(x[4]);
        const double swingLimitZ = fabs(x[5]);
        const double clampedLimitY = AZ::GetClamp(swingLimitY, 0.0, aznumeric_cast<double>(AZ::Constants::Pi));
        const double clampedLimitZ = AZ::GetClamp(swingLimitZ, 0.0, aznumeric_cast<double>(AZ::Constants::Pi));
        const double tanQuarterSwingLimitY = tan(0.25f * clampedLimitY);
        const double tanQuarterSwingLimitZ = tan(0.25f * clampedLimitZ);

        // the optimizer tries to minimize a sum of two terms:
        // - a violation term, which adds a penalty if the provided local rotation samples go outside the limit cone
        // - a volume term, which tries to make the cone as small as possible (otherwise the violation term could
        // trivially be minimized by making the cone very large)

        // violation term
        double objectiveViolation = 0.0;
        const size_t numPoses = m_localRotationSamples.size();
        if (numPoses > 0)
        {
            for (const auto& localRotationSample : m_localRotationSamples)
            {
                objectiveViolation +=
                    SwingValues(parentLocalConjugate * localRotationSample).GetViolation(tanQuarterSwingLimitY, tanQuarterSwingLimitZ);
            }
            objectiveViolation /= double(numPoses);
        }

        // volume term
        const double objectiveVolume = 0.1 * swingLimitY * swingLimitZ;

        const double weightViolation = 1.0;
        const double weightVolume = 1.0;
        if (debug)
        {
            AZ_Printf("Joint limit fitter", "limit violation term: %f, volume term: %f", objectiveViolation, objectiveVolume);
        }
        const double totalObjective = weightViolation * objectiveViolation + weightVolume * objectiveVolume;

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
