/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/NamedEntityId.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MatrixMxN.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/VectorN.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>

namespace ScriptCanvas
{
    namespace Data
    {
        // Note: Changing the order of number of values in this list almost certainly invalidates previous data
        enum class eType : AZ::u32
        {
            Boolean,
            EntityID,
            Invalid,
            Number,
            BehaviorContextObject,
            String,
            Quaternion,
            Transform,
            Vector3,
            Vector2,
            Vector4,
            AABB,
            Color,
            CRC,
            Matrix3x3,
            Matrix4x4,
            OBB,
            Plane,
            NamedEntityID,
            // Function,
            // List,
            AssetId,
            VectorN,
            MatrixMxN,
            
            // Add any new types above this. This is used to provide a count of all the types defined.
            Count
        };

        using AABBType = AZ::Aabb;
        using AssetIdType = AZ::Data::AssetId;
        using BooleanType = bool;
        using CRCType = AZ::Crc32;
        using ColorType = AZ::Color;
        using EntityIDType = AZ::EntityId;
        using NamedEntityIDType = AZ::NamedEntityId;
        using Matrix3x3Type = AZ::Matrix3x3;
        using Matrix4x4Type = AZ::Matrix4x4;
        using MatrixMxNType = AZ::MatrixMxN;
        using NumberType = double;
        using OBBType = AZ::Obb;
        using PlaneType = AZ::Plane;
        using QuaternionType = AZ::Quaternion;
        using StringType = AZStd::string;
        using TransformType = AZ::Transform;
        using Vector2Type = AZ::Vector2;
        using Vector3Type = AZ::Vector3;
        using Vector4Type = AZ::Vector4;
        using VectorNType = AZ::VectorN;

        class Type final
        {
        public:
            AZ_TYPE_INFO(Type, "{0EADF8F5-8AB8-42E9-9C50-F5C78255C817}");

            static void Reflect(AZ::ReflectContext* reflection);

            static Type AABB();
            static Type AssetId();
            static Type BehaviorContextObject(const AZ::Uuid& aztype);
            static Type Boolean();
            static Type Color();
            static Type CRC();
            static Type EntityID();
            static Type NamedEntityID();
            static Type Invalid();
            static Type Matrix3x3();
            static Type Matrix4x4();
            static Type MatrixMxN();
            static Type Number();
            static Type OBB();
            static Type Plane();
            static Type Quaternion();
            static Type String();
            static Type Transform();
            static Type Vector2();
            static Type Vector3();
            static Type Vector4();
            static Type VectorN();

            // the default ctr produces the invalid type, and is only here to help the compiler
            Type();

            AZ::Uuid GetAZType() const;
            eType GetType() const;
            bool IsValid() const;

            // returns true if this type is, or is derived from the other type
            bool IS_A(const Type& other) const;
            bool IS_EXACTLY_A(const Type& other) const;

            bool IsConvertibleFrom(const AZ::Uuid& target) const;
            bool IsConvertibleFrom(const Type& target) const;

            bool IsConvertibleTo(const AZ::Uuid& target) const;
            bool IsConvertibleTo(const Type& target) const;

            explicit operator bool() const;
            bool operator!() const;
            bool operator==(const Type& other) const;
            bool operator!=(const Type& other) const;
            Type& operator=(const Type& source) = default;

        private:
            eType m_type;
            AZ::Uuid m_azType;

            explicit Type(eType type);
            // for BehaviorContextObjects specifically
            explicit Type(const AZ::Uuid& aztype);
        };
    } // namespace Data
} // namespace ScriptCanvas
