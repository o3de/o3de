/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Base Class of all Physics Joints that will be simulated.
    struct JointConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::JointConfiguration, "{DF91D39A-4901-48C4-9159-93FD2ACA5252}");
        static void Reflect(AZ::ReflectContext* context);

        JointConfiguration() = default;
        virtual ~JointConfiguration() = default;

        // Visibility helpers for use in the Editor when reflected.
        enum PropertyVisibility : AZ::u8
        {
            ParentLocalRotation = 1 << 0, //!< Whether the parent local rotation is visible.
            ParentLocalPosition = 1 << 1, //!< Whether the parent local position is visible.
            ChildLocalRotation = 1 << 2, //!< Whether the child local rotation is visible.
            ChildLocalPosition = 1 << 3, //!< Whether the child local position is visible.
            StartSimulationEnabled = 1 << 4 //!< Whether the start simulation enabled setting is visible.
        };

        AZ::Crc32 GetPropertyVisibility(PropertyVisibility property) const;
        void SetPropertyVisibility(PropertyVisibility property, bool isVisible);

        AZ::Crc32 GetParentLocalRotationVisibility() const;
        AZ::Crc32 GetParentLocalPositionVisibility() const;
        AZ::Crc32 GetChildLocalRotationVisibility() const;
        AZ::Crc32 GetChildLocalPositionVisibility() const;
        AZ::Crc32 GetStartSimulationEnabledVisibility() const;

        // Allow additional values specific to a particular physics backend to be retrieved and modified.
        virtual AZStd::optional<float> GetPropertyValue([[maybe_unused]] const AZ::Name& propertyName)
        {
            return AZStd::nullopt;
        }
        virtual void SetPropertyValue([[maybe_unused]] const AZ::Name& propertyName, [[maybe_unused]] float value)
        {
        }

        // Entity/object association.
        void* m_customUserData = nullptr;

        // Basic initial settings.
        AZ::Quaternion m_parentLocalRotation = AZ::Quaternion::CreateIdentity(); ///< Parent joint frame relative to parent body.
        AZ::Vector3 m_parentLocalPosition = AZ::Vector3::CreateZero(); ///< Joint position relative to parent body.
        AZ::Quaternion m_childLocalRotation = AZ::Quaternion::CreateIdentity(); ///< Child joint frame relative to child body.
        AZ::Vector3 m_childLocalPosition = AZ::Vector3::CreateZero(); ///< Joint position relative to child body.
        bool m_startSimulationEnabled = true;

        // For debugging/tracking purposes only.
        AZStd::string m_debugName;

        // Default all visibility settings to invisible, since most joint configurations don't need to display these.
        AZ::u8 m_propertyVisibilityFlags = 0;
    };

    //! Default colors, line widths etc for use when visualizing joints.
    namespace JointVisualizationDefaults
    {
        inline const float Alpha = 0.6f;
        inline const AZ::Color ColorDefault = AZ::Color(1.0f, 1.0f, 1.0f, Alpha);
        inline const AZ::Color ColorFirst = AZ::Color(1.0f, 0.0f, 0.0f, Alpha);
        inline const AZ::Color ColorSecond = AZ::Color(0.0f, 1.0f, 0.0f, Alpha);
        inline const AZ::Color ColorSweepArc = AZ::Color(1.0f, 1.0f, 1.0f, Alpha);

        inline const float SweepLineDisplaceFactor = 0.5f;
        inline const float SweepLineThickness = 1.0f;
        inline const float SweepLineGranularity = 1.0f;
    } // namespace JointVisualizationDefaults
} // namespace AzPhysics
