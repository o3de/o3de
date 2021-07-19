/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_UTILS_MATH_MARSHAL
#define GM_UTILS_MATH_MARSHAL

#include <GridMate/Serialize/DataMarshal.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>

namespace GridMate
{
    /**
    * Vector2 Marshaler uses 8 bytes.
    */
    template<>
    class Marshaler<AZ::Vector2>
    {
    public:
        AZ_TYPE_INFO_LEGACY( Marshaler, "{CF906A15-B46E-4468-9C7C-EC5F5A544F78}", AZ::Vector2 );

        typedef AZ::Vector2 DataType;

        static constexpr AZStd::size_t MarshalSize = sizeof(float) * 2;

        void Marshal(WriteBuffer& wb, const AZ::Vector2& vec) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, vec.GetX());
            marshaler.Marshal(wb, vec.GetY());
        }
        void Unmarshal(AZ::Vector2& vec, ReadBuffer& rb) const
        {
            float x, y;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            vec.Set(x, y);
        }
    };
    
    /**
    * Vector3 Marshaler uses 12 bytes.
    */
    template<>
    class Marshaler<AZ::Vector3>
    {
    public:
        AZ_TYPE_INFO_LEGACY( Marshaler, "{E271F6BE-ACE0-4CAA-BE81-B4FE932A5EE8}", AZ::Vector3 );

        typedef AZ::Vector3 DataType;

        static constexpr AZStd::size_t MarshalSize = sizeof(float) * 3;

        void Marshal(WriteBuffer& wb, const AZ::Vector3& vec) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, vec.GetX());
            marshaler.Marshal(wb, vec.GetY());
            marshaler.Marshal(wb, vec.GetZ());
        }
        void Unmarshal(AZ::Vector3& vec, ReadBuffer& rb) const
        {
            float x, y, z;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            vec.Set(x, y, z);
        }
    };

    /**
    * Vector4 Marshaler uses 16 bytes.
    */
    template<>
    class Marshaler<AZ::Vector4>
    {
    public:
        typedef AZ::Vector4 DataType;

        static constexpr AZStd::size_t MarshalSize = sizeof(float) * 4;

        void Marshal(WriteBuffer& wb, const AZ::Vector4& vec) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, vec.GetX());
            marshaler.Marshal(wb, vec.GetY());
            marshaler.Marshal(wb, vec.GetZ());
            marshaler.Marshal(wb, vec.GetW());
        }
        void Unmarshal(AZ::Vector4& vec, ReadBuffer& rb) const
        {
            float x, y, z, w;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            marshaler.Unmarshal(w, rb);
            vec.Set(x, y, z, w);
        }
    };
    /**
    * Color Marshaler uses 16 bytes.
    */
    template<>
    class Marshaler<AZ::Color>
    {
    public:
        AZ_TYPE_INFO_LEGACY( Marshaler, "{4F436B7E-B770-476B-8BA5-14BC3CA17FC0}", AZ::Color );

        typedef AZ::Color DataType;

        static constexpr AZStd::size_t MarshalSize = sizeof(float) * 4;

        void Marshal(WriteBuffer& wb, const AZ::Color& color) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, color.GetR());
            marshaler.Marshal(wb, color.GetG());
            marshaler.Marshal(wb, color.GetB());
            marshaler.Marshal(wb, color.GetA());
        }
        void Unmarshal(AZ::Color& color, ReadBuffer& rb) const
        {
            float r, g, b, a;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(r, rb);
            marshaler.Unmarshal(g, rb);
            marshaler.Unmarshal(b, rb);
            marshaler.Unmarshal(a, rb);
            color.Set(r, g, b, a);
        }
    };

    /**
    * Quaternion Marshaler uses 16 bytes
    */
    template<>
    class Marshaler<AZ::Quaternion>
    {
    public:
        AZ_TYPE_INFO_LEGACY( Marshaler, "{C4D41B7B-3486-49A6-89CC-73351DF11B50}", AZ::Quaternion );

        typedef AZ::Quaternion DataType;

        static constexpr AZStd::size_t MarshalSize = sizeof(float) * 4;

        void Marshal(WriteBuffer& wb, const AZ::Quaternion& quat) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, quat.GetX());
            marshaler.Marshal(wb, quat.GetY());
            marshaler.Marshal(wb, quat.GetZ());
            marshaler.Marshal(wb, quat.GetW());
        }
        void Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const
        {
            float x, y, z, w;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            marshaler.Unmarshal(w, rb);
            quat.Set(x, y, z, w);
        }
    };

    /**
    * Transform Marshaler
    */
    template<>
    class Marshaler<AZ::Transform>
    {
    public:
        AZ_TYPE_INFO_LEGACY( Marshaler, "{6F81A4D3-8816-4080-A9BE-C86DA3884E9A}", AZ::Transform );

        typedef AZ::Transform DataType;

        static constexpr AZStd::size_t MarshalSize = Marshaler<AZ::Vector3>::MarshalSize * 4;

        void Marshal(WriteBuffer& wb, const AZ::Transform& value) const
        {
            Marshaler<AZ::Vector3> marshaler;
            marshaler.Marshal(wb, value.GetBasisX());
            marshaler.Marshal(wb, value.GetBasisY());
            marshaler.Marshal(wb, value.GetBasisZ());
            marshaler.Marshal(wb, value.GetTranslation());
        }
        void Unmarshal(AZ::Transform& value, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector3> marshaler;
            AZ::Vector3 x, y, z, pos;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            marshaler.Unmarshal(pos, rb);
            AZ::Matrix3x3 matrix3x3;
            matrix3x3.SetColumns(x, y, z);
            value = AZ::Transform::CreateFromMatrix3x3AndTranslation(matrix3x3, pos);
        }
    };

    /**
    * Matrix3x3 Marshaler
    */
    template<>
    class Marshaler<AZ::Matrix3x3>
    {
    public:
        AZ_TYPE_INFO_LEGACY( Marshaler, "{F4F8CABA-F32F-49DE-8C86-B12BA55944A2}", AZ::Matrix3x3 );

        typedef AZ::Matrix3x3 DataType;

        static constexpr AZStd::size_t MarshalSize = Marshaler<AZ::Vector3>::MarshalSize * 3;

        void Marshal(WriteBuffer& wb, const AZ::Matrix3x3& value) const
        {
            Marshaler<AZ::Vector3> marshaler;
            marshaler.Marshal(wb, value.GetBasisX());
            marshaler.Marshal(wb, value.GetBasisY());
            marshaler.Marshal(wb, value.GetBasisZ());
        }
        void Unmarshal(AZ::Matrix3x3& value, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector3> marshaler;
            AZ::Vector3 x, y, z;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            value.SetBasis(x, y, z);
        }
    };

    /**
    * Matrix4x4 Marshaler
    */
    template<>
    class Marshaler<AZ::Matrix4x4>
    {
    public:
        typedef AZ::Matrix4x4 DataType;

        static constexpr AZStd::size_t MarshalSize = Marshaler<AZ::Vector4>::MarshalSize * 4;

        void Marshal(WriteBuffer& wb, const AZ::Matrix4x4& value) const
        {
            Marshaler<AZ::Vector4> marshaler;
            marshaler.Marshal(wb, value.GetBasisX());
            marshaler.Marshal(wb, value.GetBasisY());
            marshaler.Marshal(wb, value.GetBasisZ());
            marshaler.Marshal(wb, value.GetColumn(3));
        }
        void Unmarshal(AZ::Matrix4x4& value, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector4> marshaler;
            AZ::Vector4 x, y, z, pos;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            marshaler.Unmarshal(pos, rb);
            value.SetBasisAndTranslation(x, y, z, pos);
        }
    };
}

#endif // GM_UTILS_MATH_MARSHAL
