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
    //! The min/max-scaler can be used to normalize the feature matrix, the query vector and other data.
    class MinMaxScaler
        : public FeatureMatrixTransformer
    {
    public:
        AZ_RTTI(MinMaxScaler, "{95D5BBA7-6144-4219-82F0-34C2DAB7DD3E}", FeatureMatrixTransformer);
        AZ_CLASS_ALLOCATOR_DECL

        MinMaxScaler() = default;

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

        const AZStd::vector<float>& GetMin() const { return m_dataMin; }
        const AZStd::vector<float>& GetMax() const { return m_dataMax; }

        static constexpr float s_epsilon = AZ::Constants::FloatEpsilon;

        void SaveMinMaxAsCsv(const char* filename, const AZStd::vector<AZStd::string>& columnNames = {});

    private:
        AZStd::vector<float> m_dataMin; //!< Minimum value per column seen in the given input feature matrix.
        AZStd::vector<float> m_dataMax; //!< Maximum value per column seen in the given input feature matrix.
        AZStd::vector<float> m_dataRange; //!< Per column range (m_dataMax[col] - m_dataMin[col]) seen in the given input feature matrix.

        bool m_clip = false; //!< Clip transformed values out of feature range to provided feature range.

        //! Desired range of the transformed data.
        float m_featureMin = 0.0f;
        float m_featureMax = 1.0f;
        float m_featureRange = 1.0f;
    };
} // namespace EMotionFX::MotionMatching
