/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ::SceneAPI
{
    class SCENE_CORE_API CoordinateSystemConverter
    {
        public:
            CoordinateSystemConverter();

            static const CoordinateSystemConverter CreateFromBasisVectors(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3]);
            static const CoordinateSystemConverter CreateFromTransforms(const AZ::Transform& sourceTransform, const AZ::Transform& targetTransform, const AZ::u32 targetBasisIndices[3]);

            // Inline get data.
            bool IsConversionNeeded() const                              { return m_needsConversion; }
            bool IsSourceRightHanded() const                             { return m_sourceRightHanded; }
            bool IsTargetRightHanded() const                             { return m_targetRightHanded; }
            const AZ::Transform GetSourceTransform() const               { return m_sourceTransform; }
            const AZ::Transform GetTargetTransform() const               { return m_targetTransform; }
            const AZ::Transform GetConversionTransform() const           { return m_conversionTransform; }
            const AZ::Transform GetInverseConversionTransform() const    { return m_conversionTransformInversed; }

            // Conversion methods.
            AZ::Quaternion ConvertQuaternion(const AZ::Quaternion& input) const;
            AZ::Vector3 ConvertVector3(const AZ::Vector3& input) const;
            AZ::Vector3 ConvertScale(const AZ::Vector3& input) const;
            AZ::Transform ConvertTransform(const AZ::Transform& input) const;
            AZ::Matrix3x4 ConvertMatrix3x4(const AZ::Matrix3x4 & input) const;

            // Inverse conversion methods.
            AZ::Quaternion InverseConvertQuaternion(const AZ::Quaternion& input) const;
            AZ::Vector3 InverseConvertVector3(const AZ::Vector3& input) const;
            AZ::Vector3 InverseConvertScale(const AZ::Vector3& input) const;
            AZ::Transform InverseConvertTransform(const AZ::Transform& input) const;

            // Helper methods.
            bool CheckIfIsRightHanded(const AZ::Transform& transform) const;

        private:
            AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
            AZ::Transform   m_sourceTransform;
            AZ::Transform   m_targetTransform;
            AZ::Transform   m_conversionTransform;
            AZ::Transform   m_conversionTransformInversed;
            AZ_POP_DISABLE_OVERRIDE_WARNING
            AZ::u32         m_targetBasisIndices[3];
            bool            m_needsConversion;
            bool            m_sourceRightHanded;
            bool            m_targetRightHanded;

            CoordinateSystemConverter(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3]);
    };
} // namespace AZ::SceneAPI
