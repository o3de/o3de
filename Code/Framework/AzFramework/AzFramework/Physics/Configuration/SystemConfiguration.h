/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Material.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Contains global physics settings.
    //! Used to initialize the Physics System.
    struct SystemConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::SystemConfiguration, "{24697CAF-AC00-443D-9C27-28D58734A84C}");
        static void Reflect(AZ::ReflectContext* context);

        SystemConfiguration() = default;
        virtual ~SystemConfiguration() = default;

        static constexpr float DefaultFixedTimestep = 0.0166667f; //! Value represents 1/60th or 60 FPS.

        float m_maxTimestep = 0.1f; //!< Maximum fixed timestep in seconds to run the physics update (10FPS).
        float m_fixedTimestep = DefaultFixedTimestep; //!< Timestep in seconds to run the physics update. See DefaultFixedTimestep.

        AZ::u64 m_raycastBufferSize = 32; //!< Maximum number of hits that will be returned from a raycast.
        AZ::u64 m_shapecastBufferSize = 32; //!< Maximum number of hits that can be returned from a shapecast.
        AZ::u64 m_overlapBufferSize = 32; //!< Maximum number of overlaps that can be returned from an overlap query.

        //! Contains the default global collision layers and groups.
        //! Each Physics Scene uses this as a base and will override as needed.
        CollisionConfiguration m_collisionConfig;

        Physics::DefaultMaterialConfiguration m_defaultMaterialConfiguration; //!< Default material parameters for the project.
        AZ::Data::Asset<Physics::MaterialLibraryAsset> m_materialLibraryAsset = AZ::Data::AssetLoadBehavior::NoLoad; //!< Material Library exposed by the system component SystemBus API.

        //! Controls whether the Physics System will self register to the TickBus and call StartSimulation / FinishSimulation on each Scene.
        //! Disable this to manually control Physics Scene simulation logic.
        bool m_autoManageSimulationUpdate = true;

        bool operator==(const SystemConfiguration& other) const;
        bool operator!=(const SystemConfiguration& other) const;

    private:
        // helpers for edit context
        AZ::u32 OnMaxTimeStepChanged();
        float GetFixedTimeStepMax() const;
    };
}
