/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/span.h>
#include <Allocators.h>
#include <FeatureMatrix.h>

namespace EMotionFX::MotionMatching
{
    //! Transformers can be used to e.g. normalize or scale features in the feature matrix or the query vector.
    class FeatureMatrixTransformer
    {
    public:
        AZ_RTTI(FeatureMatrixTransformer, "{B19CDBB8-FA99-4CBD-86C1-640A3CC5988A}");
        AZ_CLASS_ALLOCATOR(FeatureMatrixTransformer, MotionMatchAllocator);

        virtual ~FeatureMatrixTransformer() = default;

        struct Settings
        {
            float m_featureMin = -1.0f; //!< Minimum value after the transformation.
            float m_featureMax = 1.0f; //!< Maximum value after the transformation.

            //! Depending on the transformer to be used there might be some outliers from range [m_featureMin, m_featureMax].
            //! Clips the values to range [m_featureMin, m_featureMax] in case true or keeps the data untouched in case of false.
            bool m_clip = false;
        };

        //! Prepare the transformer.
        //! This might e.g. run some statistical analysis and cache values that will be needed for actually transforming the data.
        virtual bool Fit(const FeatureMatrix& featureMatrix, const Settings& settings) = 0;

        //! Copy and transform the input data.
        //! Note: Use the version that can batch transform the most data.
        //!       Expect significant performance losses when calling the granular versions on lots of data points.
        virtual float Transform(float value, FeatureMatrix::Index column) const = 0;
        virtual AZ::Vector2 Transform(const AZ::Vector2& value, FeatureMatrix::Index column) const = 0;
        virtual AZ::Vector3 Transform(const AZ::Vector3& value, FeatureMatrix::Index column) const = 0;
        virtual void Transform(AZStd::span<float> data) const = 0;
        virtual FeatureMatrix Transform(const FeatureMatrix& in) const = 0;

        //! Input: Already transformed data, Output: Inverse transformed data (should match data before transform)
        virtual float InverseTransform(float value, FeatureMatrix::Index column) const = 0;
        virtual AZ::Vector2 InverseTransform(const AZ::Vector2& value, FeatureMatrix::Index column) const = 0;
        virtual AZ::Vector3 InverseTransform(const AZ::Vector3& value, FeatureMatrix::Index column) const = 0;
        virtual FeatureMatrix InverseTransform(const FeatureMatrix& in) const = 0;
    };
} // namespace EMotionFX::MotionMatching
