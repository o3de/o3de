/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Allocators.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <FeatureMatrixStandardScaler.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(StandardScaler, MotionMatchAllocator);

    bool StandardScaler::Fit(const FeatureMatrix& featureMatrix, [[maybe_unused]] const Settings& settings)
    {
        const FeatureMatrix::Index numRows = featureMatrix.rows();
        const FeatureMatrix::Index numColumns = featureMatrix.cols();

        m_means.clear();
        m_means.resize(numColumns);
        m_standardDeviations.clear();
        m_standardDeviations.resize(numColumns);

        for (FeatureMatrix::Index column = 0; column < numColumns; ++column)
        {
            // Calculate mean value
            float accumulated = 0.0f;
            for (FeatureMatrix::Index row = 0; row < numRows; ++row)
            {
                accumulated += featureMatrix(row, column);
            }
            const float mean = accumulated / numRows;
            m_means[column] = mean;

            // Calculate the standard deviation
            float rss = 0.0f; // Residual sum of squares
            for (FeatureMatrix::Index row = 0; row < numRows; ++row)
            {
                const float value = featureMatrix(row, column);
                rss += (value - mean) * (value - mean);
            }
            const float variance = rss / numRows;
            const float standardDeviation = sqrtf(variance);
            m_standardDeviations[column] = standardDeviation;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    float StandardScaler::Transform(float value, FeatureMatrix::Index column) const
    {
        const float mean = m_means[column];
        float standardDeviation = m_standardDeviations[column];

        if (standardDeviation < s_epsilon)
        {
            standardDeviation = 1.0f;
        }

        // Subtract the mean and scale to unit variance
        return (value - mean) / standardDeviation;
    }

    AZ::Vector2 StandardScaler::Transform(const AZ::Vector2& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector2(Transform(value.GetX(), column + 0), Transform(value.GetY(), column + 1));
    }

    AZ::Vector3 StandardScaler::Transform(const AZ::Vector3& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector3(Transform(value.GetX(), column + 0), Transform(value.GetY(), column + 1), Transform(value.GetZ(), column + 2));
    }

    void StandardScaler::Transform(AZStd::span<float> data) const
    {
        const size_t numValues = data.size();
        AZ_Assert(numValues == m_means.size(), "Input data needs to have the same number of elements.");
        for (size_t i = 0; i < numValues; ++i)
        {
            data[i] = Transform(data[i], i);
        }
    }

    FeatureMatrix StandardScaler::Transform(const FeatureMatrix& featureMatrix) const
    {
        const FeatureMatrix::Index numRows = featureMatrix.rows();
        const FeatureMatrix::Index numColumns = featureMatrix.cols();
        FeatureMatrix result;
        result.resize(numRows, numColumns);

        for (FeatureMatrix::Index row = 0; row < numRows; ++row)
        {
            for (FeatureMatrix::Index column = 0; column < numColumns; ++column)
            {
                result(row, column) = Transform(featureMatrix(row, column), column);
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------

    FeatureMatrix StandardScaler::InverseTransform(const FeatureMatrix& featureMatrix) const
    {
        const FeatureMatrix::Index numRows = featureMatrix.rows();
        const FeatureMatrix::Index numColumns = featureMatrix.cols();
        FeatureMatrix result;
        result.resize(numRows, numColumns);

        for (FeatureMatrix::Index row = 0; row < numRows; ++row)
        {
            for (FeatureMatrix::Index column = 0; column < numColumns; ++column)
            {
                result(row, column) = InverseTransform(featureMatrix(row, column), column);
            }
        }

        return result;
    }

    AZ::Vector2 StandardScaler::InverseTransform(const AZ::Vector2& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector2(InverseTransform(value.GetX(), column + 0), InverseTransform(value.GetY(), column + 1));
    }

    AZ::Vector3 StandardScaler::InverseTransform(const AZ::Vector3& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector3(
            InverseTransform(value.GetX(), column + 0),
            InverseTransform(value.GetY(), column + 1),
            InverseTransform(value.GetZ(), column + 2));
    }

    float StandardScaler::InverseTransform(float value, FeatureMatrix::Index column) const
    {
        float standardDeviation = m_standardDeviations[column];
        if (standardDeviation < s_epsilon)
        {
            standardDeviation = 1.0f;
        }

        return value * standardDeviation + m_means[column];
    }

    void StandardScaler::SaveAsCsv(const char* filename, const AZStd::vector<AZStd::string>& columnNames)
    {
        AZStd::string data;

        // Save column names in the first row
        if (!columnNames.empty())
        {
            for (size_t i = 0; i < columnNames.size(); ++i)
            {
                if (i != 0)
                {
                    data += ",";
                }

                data += columnNames[i].c_str();
            }
            data += "\n";
        }

        for (size_t i = 0; i < m_means.size(); ++i)
        {
            if (i != 0)
            {
                data += ",";
            }

            data += AZStd::to_string(m_means[i]);
        }

        data += "\n";

        for (size_t i = 0; i < m_standardDeviations.size(); ++i)
        {
            if (i != 0)
            {
                data += ",";
            }

            data += AZStd::to_string(m_standardDeviations[i]);
        }

        data += "\n";

        AZ::IO::SystemFile file;
        if (file.Open(
                filename,
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            file.Write(data.data(), data.size());
            file.Close();
        }
    }
} // namespace EMotionFX::MotionMatching
