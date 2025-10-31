/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <Allocators.h>
#include <AzCore/std/limits.h>
#include <FeatureMatrixMinMaxScaler.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(MinMaxScaler, MotionMatchAllocator);

    bool MinMaxScaler::Fit(const FeatureMatrix& featureMatrix, const Settings& settings)
    {
        const FeatureMatrix::Index numRows = featureMatrix.rows();
        const FeatureMatrix::Index numColumns = featureMatrix.cols();

        m_clip = settings.m_clip;
        m_featureMin = settings.m_featureMin;
        m_featureMax = settings.m_featureMax;
        m_featureRange = settings.m_featureMax - settings.m_featureMin;
        AZ_Assert(m_featureRange > s_epsilon, "Feature range too small. This will lead to divisions by zero.");
        m_dataMin.clear();
        m_dataMin.resize(numColumns, AZStd::numeric_limits<float>::max());
        m_dataMax.clear();
        m_dataMax.resize(numColumns, AZStd::numeric_limits<float>::lowest());

        for (FeatureMatrix::Index row = 0; row < numRows; ++row)
        {
            for (FeatureMatrix::Index column = 0; column < numColumns; ++column)
            {
                m_dataMin[column] = AZStd::min(m_dataMin[column], featureMatrix(row, column));
                m_dataMax[column] = AZStd::max(m_dataMax[column], featureMatrix(row, column));
            }
        }

        m_dataRange.clear();
        m_dataRange.resize_no_construct(numColumns);
        for (FeatureMatrix::Index column = 0; column < numColumns; ++column)
        {
            const float columnMin = m_dataMin[column];
            const float columnMax = m_dataMax[column];
            const float range = columnMax - columnMin;
            m_dataRange[column] = range;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    float MinMaxScaler::Transform(float value, FeatureMatrix::Index column) const
    {
        const float& min = m_dataMin[column];
        const float& range = m_dataRange[column];

        float result = value;
        if (range > s_epsilon)
        {
            const float normalizedValue = (value - min) / (range);
            const float scaled = normalizedValue * m_featureRange + m_featureMin;

            result = scaled;
        }

        if (m_clip)
        {
            result = AZ::GetClamp(result, m_featureMin, m_featureMax);
        }

        return result;
    }

    AZ::Vector2 MinMaxScaler::Transform(const AZ::Vector2& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector2(
            Transform(value.GetX(), column + 0),
            Transform(value.GetY(), column + 1));
    }

    AZ::Vector3 MinMaxScaler::Transform(const AZ::Vector3& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector3(
            Transform(value.GetX(), column + 0),
            Transform(value.GetY(), column + 1),
            Transform(value.GetZ(), column + 2));
    }

    void MinMaxScaler::Transform(AZStd::span<float> data) const
    {
        const size_t numValues = data.size();
        AZ_Assert(numValues == m_dataMin.size(), "Input data needs to have the same number of elements.");
        for (size_t i = 0; i < numValues; ++i)
        {
            data[i] = Transform(data[i], i);
        }
    }

    FeatureMatrix MinMaxScaler::Transform(const FeatureMatrix& featureMatrix) const
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

    FeatureMatrix MinMaxScaler::InverseTransform(const FeatureMatrix& featureMatrix) const
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

    AZ::Vector2 MinMaxScaler::InverseTransform(const AZ::Vector2& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector2(
            InverseTransform(value.GetX(), column + 0),
            InverseTransform(value.GetY(), column + 1));
    }

    AZ::Vector3 MinMaxScaler::InverseTransform(const AZ::Vector3& value, FeatureMatrix::Index column) const
    {
        return AZ::Vector3(
            InverseTransform(value.GetX(), column + 0),
            InverseTransform(value.GetY(), column + 1),
            InverseTransform(value.GetZ(), column + 2));
    }

    float MinMaxScaler::InverseTransform(float value, FeatureMatrix::Index column) const
    {
        const float normalizedValue = (value - m_featureMin) / m_featureRange;
        return normalizedValue * m_dataRange[column] + m_dataMin[column];
    }

    void MinMaxScaler::SaveMinMaxAsCsv(const char* filename, const AZStd::vector<AZStd::string>& columnNames)
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

        for (size_t i = 0; i < m_dataMin.size(); ++i)
        {
            if (i != 0)
            {
                data += ",";
            }

            data += AZStd::to_string(m_dataMin[i]);
        }

        data += "\n";

        for (size_t i = 0; i < m_dataMax.size(); ++i)
        {
            if (i != 0)
            {
                data += ",";
            }

            data += AZStd::to_string(m_dataMax[i]);
        }

        data += "\n";

        AZ::IO::SystemFile file;
        if (file.Open(filename, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            file.Write(data.data(), data.size());
            file.Close();
        }
    }
} // namespace EMotionFX::MotionMatching
