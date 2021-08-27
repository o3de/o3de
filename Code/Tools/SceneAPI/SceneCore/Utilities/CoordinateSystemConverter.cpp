/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>
#include <AzCore/Math/Matrix3x3.h>

namespace AZ::SceneAPI
{
    // Default constructor, initializes so that it basically does no conversion.
    CoordinateSystemConverter::CoordinateSystemConverter()
        : m_sourceTransform(AZ::Transform::CreateIdentity())
        , m_targetTransform(AZ::Transform::CreateIdentity())
        , m_conversionTransform(AZ::Transform::CreateIdentity())
        , m_conversionTransformInversed(AZ::Transform::CreateIdentity())
        , m_needsConversion(false)
        , m_sourceRightHanded(CheckIfIsRightHanded(m_sourceTransform))
        , m_targetRightHanded(CheckIfIsRightHanded(m_targetTransform))
    {
        m_targetBasisIndices[0] = 0;
        m_targetBasisIndices[1] = 1;
        m_targetBasisIndices[2] = 2;
    }


    CoordinateSystemConverter::CoordinateSystemConverter(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3])
    {
        const AZ::Matrix3x3 sourceMatrix3x3 = AZ::Matrix3x3::CreateFromColumns(sourceBasisVectors[0], sourceBasisVectors[1], sourceBasisVectors[2]);
        m_sourceTransform = AZ::Transform::CreateFromMatrix3x3(sourceMatrix3x3);
        AZ_Assert(m_sourceTransform.IsOrthogonal(), "Invalid source transformation, basis vectors have to be orthogonal.");

        const AZ::Matrix3x3 targetMatrix3x3 = AZ::Matrix3x3::CreateFromColumns(targetBasisVectors[0], targetBasisVectors[1], targetBasisVectors[2]);
        m_targetTransform = AZ::Transform::CreateFromMatrix3x3(targetMatrix3x3);
        AZ_Assert(m_targetTransform.IsOrthogonal(), "Invalid target transformation, basis vectors have to be orthogonal.");

        m_conversionTransform = m_targetTransform * m_sourceTransform.GetInverse();
        m_conversionTransformInversed = m_conversionTransform.GetInverse();

        m_targetBasisIndices[0] = targetBasisIndices[0];
        m_targetBasisIndices[1] = targetBasisIndices[1];
        m_targetBasisIndices[2] = targetBasisIndices[2];

        m_needsConversion   = (m_sourceTransform != m_targetTransform);
        m_sourceRightHanded = CheckIfIsRightHanded(m_sourceTransform);
        m_targetRightHanded = CheckIfIsRightHanded(m_targetTransform);
    }


    const CoordinateSystemConverter CoordinateSystemConverter::CreateFromBasisVectors(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3])
    {
        return CoordinateSystemConverter(sourceBasisVectors, targetBasisVectors, targetBasisIndices);
    }


    const CoordinateSystemConverter CoordinateSystemConverter::CreateFromTransforms(const AZ::Transform& sourceTransform, const AZ::Transform& targetTransform, const AZ::u32 targetBasisIndices[3])
    {
        AZ::Vector3 sourceBasisVectors[3];
        sourceBasisVectors[0] = sourceTransform.GetBasisX();
        sourceBasisVectors[1] = sourceTransform.GetBasisY();
        sourceBasisVectors[2] = sourceTransform.GetBasisZ();

        AZ::Vector3 targetBasisVectors[3];
        targetBasisVectors[0] = targetTransform.GetBasisX();
        targetBasisVectors[1] = targetTransform.GetBasisY();
        targetBasisVectors[2] = targetTransform.GetBasisZ();

        return CoordinateSystemConverter(sourceBasisVectors, targetBasisVectors, targetBasisIndices);
    }


    bool CoordinateSystemConverter::CheckIfIsRightHanded(const AZ::Transform& transform) const
    {
        const AZ::Vector3 right   = transform.GetBasisX();
        const AZ::Vector3 up      = transform.GetBasisY();
        const AZ::Vector3 forward = transform.GetBasisZ();
        return ((right.Cross(up)).Dot(forward) <= 0.0f);
    }


    //-------------------------------------------------------------------------
    //  Conversions
    //-------------------------------------------------------------------------
    AZ::Quaternion CoordinateSystemConverter::ConvertQuaternion(const AZ::Quaternion& input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        const AZ::Vector3 vec = ConvertVector3( input.GetImaginary() );
        float w = input.GetW();
        if (m_sourceRightHanded != m_targetRightHanded)
        {
            w = -w;
        }

        return AZ::Quaternion(vec.GetX(), vec.GetY(), vec.GetZ(), w);
    }


    AZ::Vector3 CoordinateSystemConverter::ConvertVector3(const AZ::Vector3& input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        return m_conversionTransform.TransformPoint(input);
    }

    AZ::Transform CoordinateSystemConverter::ConvertTransform(const AZ::Transform& input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        return input * m_conversionTransform;
    }

    AZ::Matrix3x4 CoordinateSystemConverter::ConvertMatrix3x4(const AZ::Matrix3x4 & input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        return input * AZ::Matrix3x4::CreateFromTransform(m_conversionTransform);
    }


    // Convert a scale value, which never flips some axis or so, just switches them.
    // Think of two coordinate systems, where for example the Z axis is inverted in one of them, the scale will remain the same, in both systems.
    // However, if we swap Y and Z, the scale Y and Z still have to be swapped.
    AZ::Vector3 CoordinateSystemConverter::ConvertScale(const AZ::Vector3& input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        return AZ::Vector3( input.GetElement(m_targetBasisIndices[0]), 
                            input.GetElement(m_targetBasisIndices[1]), 
                            input.GetElement(m_targetBasisIndices[2]) );
    }



    //-------------------------------------------------------------------------
    //  Inverse Conversions
    //-------------------------------------------------------------------------
    AZ::Quaternion CoordinateSystemConverter::InverseConvertQuaternion(const AZ::Quaternion& input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        const AZ::Vector3 vec = InverseConvertVector3( input.GetImaginary() );
        float w = input.GetW();
        if (m_sourceRightHanded != m_targetRightHanded)
        {
            w = -w;
        }

        return AZ::Quaternion(vec.GetX(), vec.GetY(), vec.GetZ(), w);
    }


    AZ::Vector3 CoordinateSystemConverter::InverseConvertVector3(const AZ::Vector3& input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        return m_conversionTransformInversed.TransformPoint(input);
    }


    AZ::Transform CoordinateSystemConverter::InverseConvertTransform(const AZ::Transform& input) const
    {
        if (!m_needsConversion)
        {
            return input;
        }

        return input * m_conversionTransformInversed;
    }


    AZ::Vector3 CoordinateSystemConverter::InverseConvertScale(const AZ::Vector3& input) const
    {
        return ConvertScale(input);
    }
} // namespace AZ::SceneAPI
