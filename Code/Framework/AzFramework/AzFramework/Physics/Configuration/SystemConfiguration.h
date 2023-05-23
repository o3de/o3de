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

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Contains global physics settings.
    //! Used to initialize the Physics System.
    struct alignas(16) SystemConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::SystemConfiguration, "{24697CAF-AC00-443D-9C27-28D58734A84C}");
        static void Reflect(AZ::ReflectContext* context);

        SystemConfiguration() = default;
        virtual ~SystemConfiguration() = default;

        static constexpr float DefaultFixedTimestep = 0.0166667f; //! Value represents 1/60th or 60 FPS.

        float m_maxTimestep = 0.1f; //!< Maximum fixed timestep in seconds to run the physics update (10FPS).
        float m_fixedTimestep = DefaultFixedTimestep; //!< Timestep in seconds to run the physics update. See DefaultFixedTimestep.

        AZ::u32 m_raycastBufferSize = 32; //!< Maximum number of hits that will be returned from a raycast.
        AZ::u32 m_shapecastBufferSize = 32; //!< Maximum number of hits that can be returned from a shapecast.
        AZ::u32 m_overlapBufferSize = 32; //!< Maximum number of overlaps that can be returned from an overlap query.

        //! Contains the default global collision layers and groups.
        //! Each Physics Scene uses this as a base and will override as needed.
        CollisionConfiguration m_collisionConfig;

        //! Controls whether the Physics System will self register to the TickBus and call StartSimulation / FinishSimulation on each Scene.
        //! Disable this to manually control Physics Scene simulation logic.
        bool m_autoManageSimulationUpdate = true;

        bool operator==(const SystemConfiguration& other) const;
        bool operator!=(const SystemConfiguration& other) const;

    private:
        // helpers for edit context
        AZ::u32 OnMaxTimeStepChanged();
        float GetFixedTimeStepMax() const;

        // Padding with a 16 byte aligned structure to make SystemConfiguration aligned to 16 bytes too.
        // Without this, SystemConfiguration generates warnings everywhere it is used indicating that
        // padding was added. But having this structure limits the warnings to this member usage because
        // SystemConfiguration won't need extra padding to achieve 16 byte alignment.
        AZ_PUSH_DISABLE_WARNING(4324, "-Wunknown-warning-option") // structure was padded due to alignment
        [[maybe_unused]] struct alignas(16)
        {
            unsigned char m_unused[16];
        } m_unusedPadding;
        AZ_POP_DISABLE_WARNING
    };
}
