/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/functional.h>
#include <Debug/PhysXDebug.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>
#include <PxPhysicsAPI.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/time.h>

namespace PhysX
{
    namespace Debug
    {
        physx::PxPvd* PhysXDebug::InitializePhysXPvd([[maybe_unused]] physx::PxFoundation* foundation)
        {
#ifdef AZ_PHYSICS_DEBUG_ENABLED
            m_pvd = PxCreatePvd(*foundation);
            return m_pvd;
#else
            return nullptr;
#endif
        }

        void PhysXDebug::ShutdownPhysXPvd()
        {
            DisconnectFromPvd();
            if (m_pvd)
            {
                m_pvd->release();
                m_pvd = nullptr;
            }
        }

        void PhysXDebug::Initialize(const DebugConfiguration& config)
        {
            m_config = config;
        }

        void PhysXDebug::UpdateDebugConfiguration(const DebugConfiguration& config)
        {
            const bool signalDebugDataChanged = m_config.m_debugDisplayData != config.m_debugDisplayData;
            const bool signalPvdConfigChanged = m_config.m_pvdConfigurationData != config.m_pvdConfigurationData;
            if (signalDebugDataChanged || signalPvdConfigChanged)
            {
                m_config = config;
                if (signalDebugDataChanged)
                {
                    m_debugDisplayDataChangedEvent.Signal(config.m_debugDisplayData);
                }
                if (signalPvdConfigChanged)
                {
                    m_pvdConfigurationChangedEvent.Signal(config.m_pvdConfigurationData);
                }
            }
        }

        const DebugConfiguration& PhysXDebug::GetDebugConfiguration() const
        {
            return m_config;
        }

        const PvdConfiguration& PhysXDebug::GetPhysXPvdConfiguration() const
        {
            return m_config.m_pvdConfigurationData;
        }

        const DebugDisplayData& PhysXDebug::GetDebugDisplayData() const
        {
            return m_config.m_debugDisplayData;
        }

        void PhysXDebug::UpdateColliderProximityVisualization(const ColliderProximityVisualization& data)
        {
            if (m_config.m_debugDisplayData.m_colliderProximityVisualization != data)
            {
                m_config.m_debugDisplayData.m_colliderProximityVisualization = data;
                m_colliderProximityVisualizationChangedEvent.Signal(data);
            }
        }

        bool PhysXDebug::ConnectToPvd()
        {
#ifdef AZ_PHYSICS_DEBUG_ENABLED
            DisconnectFromPvd();

            // Select current PhysX Pvd debug type
            switch (m_config.m_pvdConfigurationData.m_transportType)
            {
                case PhysX::Debug::PvdTransportType::File:
                {
                    // Use current timestamp in the filename.
                    AZ::u64 currentTimeStamp = AZStd::GetTimeUTCMilliSecond() / 1000;

                    // Strip any filename used as .pxd2 forced (only .pvd or .px2 valid for PVD version 3.2016.12.21494747)
                    AzFramework::StringFunc::Path::StripExtension(m_config.m_pvdConfigurationData.m_fileName);

                    // Create output filename (format: <TimeStamp>-<FileName>.pxd2)
                    AZStd::string filename = AZStd::to_string(currentTimeStamp);
                    AzFramework::StringFunc::Append(filename, "-");
                    AzFramework::StringFunc::Append(filename, m_config.m_pvdConfigurationData.m_fileName.c_str());
                    AzFramework::StringFunc::Append(filename, ".pxd2");

                    AZStd::string rootDirectory{ AZStd::string_view(AZ::Utils::GetEnginePath()) };

                    // Create the full filepath.
                    AZStd::string safeFilePath;
                    AzFramework::StringFunc::Path::Join
                    (
                        rootDirectory.c_str(),
                        filename.c_str(),
                        safeFilePath
                    );

                    m_pvdTransport = physx::PxDefaultPvdFileTransportCreate(safeFilePath.c_str());
                    break;
                }
                case PhysX::Debug::PvdTransportType::Network:
                {
                    m_pvdTransport = physx::PxDefaultPvdSocketTransportCreate
                    (
                        m_config.m_pvdConfigurationData.m_host.c_str(),
                        m_config.m_pvdConfigurationData.m_port,
                        m_config.m_pvdConfigurationData.m_timeoutInMilliseconds
                    );
                    break;
                }
                default:
                {
                    AZ_Error("PhysX", false, "Invalid PhysX Visual Debugger (PVD) Debug Type used %d.", m_config.m_pvdConfigurationData.m_transportType);
                    break;
                }
            }

            bool pvdConnectionSuccessful = false;

            if (m_pvd && m_pvdTransport)
            {
                pvdConnectionSuccessful = m_pvd->connect(*m_pvdTransport, physx::PxPvdInstrumentationFlag::eALL);
                if (pvdConnectionSuccessful)
                {
                    AZ_Printf("PhysX", "Successfully connected to the PhysX Visual Debugger (PVD).\n");
                }
                else
                {
                    AZ_Printf("PhysX", "Failed to connect to the PhysX Visual Debugger (PVD).\n");
                }
            }

            return pvdConnectionSuccessful;
#else
            return false;
#endif // AZ_PHYSICS_DEBUG_ENABLED
        }

        void PhysXDebug::DisconnectFromPvd()
        {
#ifdef AZ_PHYSICS_DEBUG_ENABLED
            if (m_pvd)
            {
                m_pvd->disconnect();
            }

            if (m_pvdTransport)
            {
                m_pvdTransport->release();
                m_pvdTransport = nullptr;
                AZ_Printf("PhysX", "Successfully disconnected from the PhysX Visual Debugger (PVD).\n");
            }
#endif
        }
    }// namespace Debug
}// namespace PhysX
