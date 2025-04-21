/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
* @file RPISystemComponent.cpp
* @brief Contains the definition of the RPISystemComponent methods that aren't defined as inline
*/

#include <RPI.Private/RPISystemComponent.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHIUtils.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/PlatformId/PlatformId.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/Components/ConsoleBus.h>

#include <Atom/RPI.Public/PerformanceCollectionNotificationBus.h>

#ifdef RPI_EDITOR
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#endif

namespace AZ
{
    namespace RPI
    {
        void RPISystemComponent::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<RPISystemComponent, Component>()
                    ->Version(0)
                    ->Field("RpiDescriptor", &RPISystemComponent::m_rpiDescriptor)
                    ;


                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<RPISystemComponent>("Atom RPI", "Atom Renderer")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RPISystemComponent::m_rpiDescriptor, "RPI System Settings", "Settings for create RPI system")
                        ;
                }
            }

            RPISystem::Reflect(context);
        }

        void RPISystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(RHI::Factory::GetComponentService());
        }

        void RPISystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("RPISystem"));
        }

        void RPISystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("XRSystemService"));
        }

        RPISystemComponent::RPISystemComponent()
        {
        #ifdef RPI_EDITOR
            AZ_Assert(m_materialFunctorRegistration == nullptr, "Material functor registration should be initialized with nullptr. "
                "And allocated depending on the component is in editors or not.");
            m_materialFunctorRegistration = aznew MaterialFunctorSourceDataRegistration;
            m_materialFunctorRegistration->Init();
        #endif
        }

        RPISystemComponent::~RPISystemComponent()
        {
        #ifdef RPI_EDITOR
            if (m_materialFunctorRegistration)
            {
                m_materialFunctorRegistration->Shutdown();
                delete m_materialFunctorRegistration;
            }
        #endif
        }

        void RPISystemComponent::Activate()
        {
            InitializePerformanceCollector();

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            const char* settingPath = "/O3DE/Atom/RPI/Initialization";
            const char* setregName = "NullRenderer"; // same as serialization context name for RPISystemDescriptor::m_isNullRenderer

            AZ::ApplicationTypeQuery appType;
            ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
            
            bool isNullRenderer = false;
            if ( (appType.IsHeadless()) || (RHI::IsNullRHI()) )
            {
                // if the application is `headless` or the RHI is the null RHI, merge `NullRenderer` attribute to the setting registry
                isNullRenderer = true;
            }
            else
            {
                // The command-line switch "--NullRenderer=true" can also be used to switch to null renderer.  This is maintained
                // for backwards compatibility.  Use "-rhi=null" instead.
                const char* nullRendererOption = "NullRenderer"; // command line option name
                const AzFramework::CommandLine* commandLine = nullptr;
                AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);
                if (commandLine->GetNumSwitchValues(nullRendererOption) > 0)
                {
                    isNullRenderer = true;
                }
            }

            if (isNullRenderer)
            {
                // add it to setting registry
                auto overrideArg = AZStd::string::format("%s/%s=true", settingPath, setregName);
                settingsRegistry->MergeCommandLineArgument(overrideArg, "");
            }

            // load rpi desriptor from setting registry
            settingsRegistry->GetObject(m_rpiDescriptor, settingPath);

            m_rpiSystem.Initialize(m_rpiDescriptor);

            // Part of RPI system initialization requires asset system to be ready
            // which happens after the game system started
            // Use the tick bus to delay this initialization
            AZ::TickBus::QueueFunction(
                [this]()
                {
                    m_rpiSystem.InitializeSystemAssets();
                });

            AZ::SystemTickBus::Handler::BusConnect();
            AZ::RHI::RHISystemNotificationBus::Handler::BusConnect();
        }

        void RPISystemComponent::Deactivate()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            m_rpiSystem.Shutdown();
            AZ::RHI::RHISystemNotificationBus::Handler::BusDisconnect();
        }

        void RPISystemComponent::OnSystemTick()
        {
            if (m_performanceCollector)
            {
                if (m_gpuPassProfiler && !m_performanceCollector->IsWaitingBeforeCapture() && m_gpuPassProfiler->IsGpuTimeMeasurementEnabled())
                {
                    uint64_t durationNanoseconds = m_gpuPassProfiler->MeasureGpuTimeInNanoseconds(AZ::RPI::PassSystemInterface::Get()->GetRootPass());
                    // The first three frames, it is expected to be zero. So only record non-zero samples.
                    if (durationNanoseconds > 0)
                    {
                        m_performanceCollector->RecordSample(PerformanceSpecGpuTime, AZStd::chrono::microseconds(aznumeric_cast<int64_t>(durationNanoseconds / 1000)));
                    }
                }

                m_performanceCollector->RecordPeriodicEvent(PerformanceSpecEngineCpuTime);
                m_performanceCollector->FrameTick();
            }

            {
                AZ::Debug::ScopeDuration performanceScopeDuration(m_performanceCollector.get(), PerformanceSpecGraphicsSimulationTime);
                m_rpiSystem.SimulationTick();
            }

            {
                AZ::Debug::ScopeDuration performanceScopeDuration(m_performanceCollector.get(), PerformanceSpecGraphicsRenderTime);
                m_rpiSystem.RenderTick();
            }
        }
        
        void RPISystemComponent::OnDeviceRemoved([[maybe_unused]] RHI::Device* device)
        {
#if defined(AZ_FORCE_CPU_GPU_INSYNC)
            const AZStd::string errorMessage = AZStd::string::format("GPU device was removed while working on pass %s. Check the log file for more detail.", device->GetLastExecutingScope().data());
#else
            const AZStd::string errorMessage = "GPU device was removed. Check the log file for more detail.";
#endif
            if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
            {
                nativeUI->DisplayOkDialog("O3DE Fatal Error", errorMessage.c_str(), false);
            }
            else
            {
                AZ_Error("Atom", false, "O3DE Fatal Error: %s\n", errorMessage.c_str());
            }

            // Stop execution since we can't recover from device removal error
            Debug::Trace::Instance().Crash();
        }

        void RPISystemComponent::RegisterXRInterface(XRRenderingInterface* xrSystemInterface)
        {
            m_rpiSystem.RegisterXRSystem(xrSystemInterface);
        }

        void RPISystemComponent::UnRegisterXRInterface()
        {
            m_rpiSystem.UnregisterXRSystem();
        }

        AZ_CVAR_EXTERNED(AZ::u32, r_metricsNumberOfCaptureBatches);
        AZ_CVAR_EXTERNED(AZ::CVarFixedString, r_metricsDataLogType);
        AZ_CVAR_EXTERNED(AZ::u32, r_metricsWaitTimePerCaptureBatch);
        AZ_CVAR_EXTERNED(AZ::u32, r_metricsFrameCountPerCaptureBatch);
        AZ_CVAR_EXTERNED(bool, r_metricsMeasureGpuTime);
        AZ_CVAR_EXTERNED(bool, r_metricsQuitUponCompletion);

        AZStd::string RPISystemComponent::GetLogCategory()
        {
            AZStd::string platformName = AZ::GetPlatformName(AZ::g_currentPlatform);
            AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
            auto logCategory = AZStd::string::format("%.*s-%s-%s", AZ_STRING_ARG(PerformanceLogCategory), platformName.c_str(), apiName.GetCStr());
            return logCategory;
        }

        void RPISystemComponent::InitializePerformanceCollector()
        {
            auto onBatchCompleteCallback = [&](AZ::u32 pendingBatches) {
                AZ_TracePrintf("RPISystem", "Completed a performance batch, still %u batches are pending.\n", pendingBatches);
                r_metricsNumberOfCaptureBatches = pendingBatches;
                if (pendingBatches == 0)
                {
                    m_gpuPassProfiler->SetGpuTimeMeasurementEnabled(false);
                    // Force disabling timestamp collection in the root pass.
                    AZ::RPI::PassSystemInterface::Get()->GetRootPass()->SetTimestampQueryEnabled(false);
                    PerformaceCollectionNotificationBus::Broadcast(&PerformaceCollectionNotification::OnPerformanceCollectionJobFinished, m_performanceCollector->GetOutputFilePath().String());
                }
                if (r_metricsQuitUponCompletion && (pendingBatches == 0))
                {
                    AzFramework::ConsoleRequestBus::Broadcast(
                        &AzFramework::ConsoleRequests::ExecuteConsoleCommand, "quit");
                }
            };

            auto performanceMetrics = AZStd::to_array<AZStd::string_view>({
                PerformanceSpecGraphicsSimulationTime,
                PerformanceSpecGraphicsRenderTime,
                PerformanceSpecEngineCpuTime,
                PerformanceSpecGpuTime,
                });
            AZStd::string logCategory = GetLogCategory();
            m_performanceCollector = AZStd::make_unique<AZ::Debug::PerformanceCollector>(
                logCategory, performanceMetrics, onBatchCompleteCallback);
            m_gpuPassProfiler = AZStd::make_unique<GpuPassProfiler>();

            //Feed the CVAR values.
            m_gpuPassProfiler->SetGpuTimeMeasurementEnabled(r_metricsMeasureGpuTime);
            m_performanceCollector->UpdateDataLogType(GetDataLogTypeFromCVar(r_metricsDataLogType));
            m_performanceCollector->UpdateFrameCountPerCaptureBatch(r_metricsFrameCountPerCaptureBatch);
            m_performanceCollector->UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(r_metricsWaitTimePerCaptureBatch));
            m_performanceCollector->UpdateNumberOfCaptureBatches(r_metricsNumberOfCaptureBatches);
        }
    } // namespace RPI
} // namespace AZ
