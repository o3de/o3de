#pragma once

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>


namespace EMotionFX
{
    namespace Pipeline
    {

        class CoordinateSystemConverter
        {
            public:
                CoordinateSystemConverter();

                static const CoordinateSystemConverter CreateFromBasisVectors(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3]);
                static const CoordinateSystemConverter CreateFromTransforms(const AZ::Transform& sourceTransform, const AZ::Transform& targetTransform, const AZ::u32 targetBasisIndices[3]);

                // Inline get data.
                AZ_INLINE bool IsConversionNeeded() const                               { return m_needsConversion; }
                AZ_INLINE bool IsSourceRightHanded() const                              { return m_sourceRightHanded; }
                AZ_INLINE bool IsTargetRightHanded() const                              { return m_targetRightHanded; }
                AZ_INLINE const AZ::Transform& GetSourceTransform() const               { return m_sourceTransform; }
                AZ_INLINE const AZ::Transform& GetTargetTransform() const               { return m_targetTransform; }
                AZ_INLINE const AZ::Transform& GetConversionTransform() const           { return m_conversionTransform; }
                AZ_INLINE const AZ::Transform& GetInverseConversionTransform() const    { return m_conversionTransformInversed; }

                // Conversion methods.
                AZ::Quaternion ConvertQuaternion(const AZ::Quaternion& input) const;
                AZ::Vector3 ConvertVector3(const AZ::Vector3& input) const;
                AZ::Vector3 ConvertScale(const AZ::Vector3& input) const;
                AZ::Transform ConvertTransform(const AZ::Transform& input) const;

                // Inverse conversion methods.
                AZ::Quaternion InverseConvertQuaternion(const AZ::Quaternion& input) const;
                AZ::Vector3 InverseConvertVector3(const AZ::Vector3& input) const;
                AZ::Vector3 InverseConvertScale(const AZ::Vector3& input) const;
                AZ::Transform InverseConvertTransform(const AZ::Transform& input) const;

                // Helper methods.
                bool CheckIfIsRightHanded(const AZ::Transform& transform) const;

            private:
                AZ::Transform   m_sourceTransform;
                AZ::Transform   m_targetTransform;
                AZ::Transform   m_conversionTransform;
                AZ::Transform   m_conversionTransformInversed;
                AZ::u32         m_targetBasisIndices[3];
                bool            m_needsConversion;
                bool            m_sourceRightHanded;
                bool            m_targetRightHanded;

                CoordinateSystemConverter(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3]);
        };

    }   // namespace Pipeline
}   // namespace EMotionFX

