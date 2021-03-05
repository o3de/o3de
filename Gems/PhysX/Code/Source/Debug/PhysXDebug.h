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
            physx::PxPvdTransport* m_pvdTransport = nullptr;
            physx::PxPvd* m_pvd = nullptr;

            DebugConfiguration m_config;
        };
    }// namespace Debug
}// namespace PhysX
