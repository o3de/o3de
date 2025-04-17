/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <FeatureMatrix.h>
#include <FeatureMatrixTransformer.h>

namespace EMotionFX::MotionMatching
{
    //! The standard scaler can be used to normalize the feature matrix, the query vector and other data.
    //! It standardizes features by subtracting the mean and scaling to the unit variance.
    //! This implementation is mimicking the behavior of the standard scaler from scikit-learn (sklearn.preprocessing.StandardScaler).
    //! As we use floats by default, our errors are bigger, especially if the variance is small as this leads to a division by a small
    //! value. In case the calculated standard deviation for a given feature is smaller than the given s_epsilon value, the standard
    //! deviation gets force-set to 1.0 to avoid divisions by infinity and preserve the value when doing the transform -> inverse-transform
    //! roundtrip.
    class StandardScaler : public FeatureMatrixTransformer
    {
    public:
        AZ_RTTI(StandardScaler, "{A0C7F056-94B0-43A1-8D12-B1A7B67AB92A}", FeatureMatrixTransformer);
        AZ_CLASS_ALLOCATOR_DECL;

        StandardScaler() = default;

        bool Fit(const FeatureMatrix& featureMatrix, const Settings& settings = {}) override;

        // Normalize and scale the values.
        float Transform(float value, FeatureMatrix::Index column) const override;
        AZ::Vector2 Transform(const AZ::Vector2& value, FeatureMatrix::Index column) const override;
        AZ::Vector3 Transform(const AZ::Vector3& value, FeatureMatrix::Index column) const override;
        void Transform(AZStd::span<float> data) const override;
        FeatureMatrix Transform(const FeatureMatrix& featureMatrix) const override;

        // From normalized and scaled back to the original values.
        FeatureMatrix InverseTransform(const FeatureMatrix& featureMatrix) const override;
        AZ::Vector2 InverseTransform(const AZ::Vector2& value, FeatureMatrix::Index column) const override;
        AZ::Vector3 InverseTransform(const AZ::Vector3& value, FeatureMatrix::Index column) const override;
        float InverseTransform(float value, FeatureMatrix::Index column) const override;

        const AZStd::vector<float>& GetMeans() const
        {
            return m_means;
        }
        const AZStd::vector<float>& GetStandardDeviations() const
        {
            return m_standardDeviations;
        }

        static constexpr float s_epsilon = AZ::Constants::FloatEpsilon;

        void SaveAsCsv(const char* filename, const AZStd::vector<AZStd::string>& columnNames = {});

    private:
        AZStd::vector<float> m_means; //!< The mean value for each feature / column.
        AZStd::vector<float> m_standardDeviations; //!< The standard deviation for each feature / column.
    };
} // namespace EMotionFX::MotionMatching
