/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

#include <PhysX/Debug/PhysXDebugConfiguration.h>

namespace AZ
{
    class ReflectContext;
}

namespace PhysX
{
    //! PhysX wind settings.
    class WindConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_TYPE_INFO(PhysX::WindConfiguration, "{6EA3E646-ECDA-4044-912D-5722D5100066}");
        static void Reflect(AZ::ReflectContext* context);

        /// Tag value that will be used to identify entities that provide global wind value.
        /// Global wind has no bounds and affects objects across entire level.
        AZStd::string m_globalWindTag = "global_wind";
        /// Tag value that will be used to identify entities that provide local wind value.
        /// Local wind is only applied within bounds defined by PhysX collider.
        AZStd::string m_localWindTag = "wind";

        bool operator==(const WindConfiguration& other) const;
        bool operator!=(const WindConfiguration& other) const;
    };

    //! Contains global physics settings.
    //! Used to initialize the Physics System.
    struct PhysXSystemConfiguration : public AzPhysics::SystemConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(PhysX::PhysXSystemConfiguration, "{6E25A37B-2109-452C-97C9-B737CC72704F}");
        static void Reflect(AZ::ReflectContext* context);

        static PhysXSystemConfiguration CreateDefault();

        WindConfiguration m_windConfiguration; //!< Wind configuration for PhysX.

        bool operator==(const PhysXSystemConfiguration& other) const;
        bool operator!=(const PhysXSystemConfiguration& other) const;
    };
}
