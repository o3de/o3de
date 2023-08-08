/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/utils.h>

namespace ScriptCanvas
{
    namespace Data
    {
        class Type;
        enum class eType : AZ::u32;

        Type FromAZType(const AZ::Uuid& aztype);
        Type FromAZTypeChecked(const AZ::Uuid& aztype);
        AZStd::pair<bool, Type> FromAZTypeHelper(const AZ::Uuid& type);

        AZ::Uuid ToAZType(eType type);
        AZ::Uuid ToAZType(const Type& type);

        bool IsAZRttiTypeOf(const AZ::Uuid& candidate, const AZ::Uuid& reference);
        bool IS_A(const Type& candidate, const Type& reference);
        bool IS_EXACTLY_A(const Type& candidate, const Type& reference);
        bool IsConvertible(const Type& source, const AZ::Uuid& target);
        bool IsConvertible(const Type& source, const Type& target);

        bool IsAABB(const AZ::Uuid& type);
        bool IsAABB(const Type& type);
        bool IsAssetId(const AZ::Uuid& type);
        bool IsAssetId(const Type& type);
        bool IsBoolean(const AZ::Uuid& type);
        bool IsBoolean(const Type& type);
        bool IsColor(const AZ::Uuid& type);
        bool IsColor(const Type& type);
        bool IsCRC(const AZ::Uuid& type);
        bool IsCRC(const Type& type);
        bool IsEntityID(const AZ::Uuid& type);
        bool IsEntityID(const Type& type);
        bool IsNamedEntityID(const AZ::Uuid& type);
        bool IsNamedEntityID(const Type& type);
        bool IsNumber(const AZ::Uuid& type);
        bool IsNumber(const Type& type);
        bool IsMatrix3x3(const AZ::Uuid& type);
        bool IsMatrix3x3(const Type& type);
        bool IsMatrix4x4(const AZ::Uuid& type);
        bool IsMatrix4x4(const Type& type);
        bool IsMatrixMxN(const AZ::Uuid& type);
        bool IsMatrixMxN(const Type& type);
        bool IsOBB(const AZ::Uuid& type);
        bool IsOBB(const Type& type);
        bool IsPlane(const AZ::Uuid& type);
        bool IsPlane(const Type& type);
        bool IsQuaternion(const AZ::Uuid& type);
        bool IsQuaternion(const Type& type);
        bool IsString(const AZ::Uuid& type);
        bool IsString(const Type& type);
        bool IsTransform(const AZ::Uuid& type);
        bool IsTransform(const Type& type);
        bool IsVector2(const AZ::Uuid& type);
        bool IsVector2(const Type& type);
        bool IsVector3(const AZ::Uuid& type);
        bool IsVector3(const Type& type);
        bool IsVector4(const AZ::Uuid& type);
        bool IsVector4(const Type& type);
        bool IsVectorN(const AZ::Uuid& type);
        bool IsVectorN(const Type& type);

        bool IsVectorType(const AZ::Uuid& type);
        bool IsVectorType(const Type& type);

        bool IsAutoBoxedType(const Type& type);
        bool IsValueType(const Type& type);

        bool IsContainerType(const AZ::Uuid& type);
        bool IsContainerType(const Type& type);
        bool IsMapContainerType(const AZ::Uuid& type);
        bool IsMapContainerType(const Type& type);
        bool IsOutcomeType(const AZ::Uuid& type);
        bool IsOutcomeType(const Type& type);
        bool IsSetContainerType(const AZ::Uuid& type);
        bool IsSetContainerType(const Type& type);
        bool IsVectorContainerType(const AZ::Uuid& type);
        bool IsVectorContainerType(const Type& type);

        bool IsSupportedBehaviorContextObject(const AZ::Uuid& type);
    } // namespace Data
} // namespace ScriptCanvas
