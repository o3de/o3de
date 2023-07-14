/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/tuple.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace MatrixMxNFunctions
    {
        Data::MatrixMxNType Zero(Data::NumberType rows, Data::NumberType cols);

        Data::MatrixMxNType Random(Data::NumberType rows, Data::NumberType cols);

        Data::NumberType GetRowCount(const Data::MatrixMxNType& source);

        Data::NumberType GetColumnCount(const Data::MatrixMxNType& source);

        Data::NumberType GetElement(const Data::MatrixMxNType& source, Data::NumberType row, Data::NumberType col);

        Data::MatrixMxNType Transpose(const Data::MatrixMxNType& source);

        Data::VectorNType MultiplyByVector(const Data::MatrixMxNType& lhs, const Data::VectorNType& rhs);

        Data::MatrixMxNType MultiplyByMatrix(const Data::MatrixMxNType& lhs, const Data::MatrixMxNType& rhs);
    } // namespace MatrixMxNFunctions
} // namespace ScriptCanvas
