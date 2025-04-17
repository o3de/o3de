/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <iostream>
#include <fstream>

#include <Allocators.h>
#include <FeatureMatrix.h>
#include <FeatureSchema.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeatureMatrix, MotionMatchAllocator)

    void FeatureMatrix::Clear()
    {
        resize(0, 0);
    }

    void FeatureMatrix::SaveAsCsv(const AZStd::string& filename, const AZStd::vector<AZStd::string>& columnNames) const
    {
        std::ofstream file(filename.c_str());

        // Save column names in the first row
        if (!columnNames.empty())
        {
            for (size_t i = 0; i < columnNames.size(); ++i)
            {
                if (i != 0)
                {
                    file << ",";
                }

                file << columnNames[i].c_str();
            }
            file << "\n";
        }

        // Save coefficients
#ifdef O3DE_USE_EIGEN
        // Force specify precision, else wise values close to 0.0 get rounded to 0.0.
        const static Eigen::IOFormat csvFormat(/*Eigen::StreamPrecision|FullPrecision*/8, Eigen::DontAlignCols, ", ", "\n");
        file << format(csvFormat);
#endif
    }

    void FeatureMatrix::SaveAsCsv(const AZStd::string& filename, const FeatureSchema* featureSchema) const
    {
        const AZStd::vector<AZStd::string> columnNames = featureSchema->CollectColumnNames();
        SaveAsCsv(filename, columnNames);
    }

    AZ::Vector2 FeatureMatrix::GetVector2(Index row, Index startColumn) const
    {
        return AZ::Vector2(
            coeff(row, startColumn + 0),
            coeff(row, startColumn + 1));
    }

    void FeatureMatrix::SetVector2(Index row, Index startColumn, const AZ::Vector2& value)
    {
        operator()(row, startColumn + 0) = value.GetX();
        operator()(row, startColumn + 1) = value.GetY();
    }

    AZ::Vector3 FeatureMatrix::GetVector3(Index row, Index startColumn) const
    {
        return AZ::Vector3(
            coeff(row, startColumn + 0),
            coeff(row, startColumn + 1),
            coeff(row, startColumn + 2));
    }

    void FeatureMatrix::SetVector3(Index row, Index startColumn, const AZ::Vector3& value)
    {
        operator()(row, startColumn + 0) = value.GetX();
        operator()(row, startColumn + 1) = value.GetY();
        operator()(row, startColumn + 2) = value.GetZ();
    }

    size_t FeatureMatrix::CalcMemoryUsageInBytes() const
    {
        const size_t bytesPerValue = sizeof(O3DE_MM_FLOATTYPE);
        const size_t numValues = size();
        return numValues * bytesPerValue;
    }
} // namespace EMotionFX::MotionMatching
