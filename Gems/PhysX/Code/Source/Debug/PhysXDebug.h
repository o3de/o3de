/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <PhysX/Debug/PhysXDebugInterface.h>

namespace physx
{
    class PxFoundation;
    class PxPvdTransport;
    class PxPvd;
}

namespace PhysX
{
    namespace Debug
    {
        //! Implementation of the PhysXDebugInterface.
        class PhysXDebug
            : public AZ::Interface<PhysXDebugInterface>::Registrar
        {
        public:
            PhysXDebug() = default;

            physx::PxPvd* InitializePhysXPvd(physx::PxFoundation* foundation);
            void ShutdownPhysXPvd();

            // PhysXDebugInterface ...
            void Initialize(const DebugConfiguration& config) override;
            void UpdateDebugConfiguration(const DebugConfiguration& config) override;
            const DebugConfiguration& GetDebugConfiguration() const override;
            const PvdConfiguration& GetPhysXPvdConfiguration() const override;
            const DebugDisplayData& GetDebugDisplayData() const override;
            void UpdateColliderProximityVisualization(const ColliderProximityVisualization& data) override;
            bool ConnectToPvd() override;
            void DisconnectFromPvd() override;

        private:
            [[maybe_unused]] physx::PxPvdTransport* m_pvdTransport = nullptr;
            physx::PxPvd* m_pvd = nullptr;

            DebugConfiguration m_config;
        };
    }// namespace Debug
}// namespace PhysX
