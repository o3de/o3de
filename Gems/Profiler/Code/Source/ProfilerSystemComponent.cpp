/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProfilerSystemComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Profiler
{
    static constexpr AZ::Crc32 profilerServiceCrc = AZ_CRC_CE("ProfilerService");

    struct DeplayedFunction
    {
        using func_type = AZStd::function<void()>;

        DeplayedFunction(int framesToDelay, func_type&& function)
            : m_function(AZStd::move(function))
            , m_framesLeft(framesToDelay)
        {
        }

        void Run()
        {
            if (--m_framesLeft <= 0)
            {
                m_function();
            }
            else
            {
                AZ::SystemTickBus::QueueFunction(
                    [](DeplayedFunction&& delayedFunc)
                    {
                        delayedFunc.Run();
                    },
                    AZStd::move(*this)
                );
            }
        }

        func_type m_function;
        int m_framesLeft{ 0 };
    };

    class ProfilerNotificationBusHandler final
        : public ProfilerNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ProfilerNotificationBusHandler, "{44161459-B816-4876-95A4-BA16DEC767D6}", AZ::SystemAllocator,
            OnCaptureCpuProfilingStatisticsFinished
        );

        void OnCaptureCpuProfilingStatisticsFinished(bool result, const AZStd::string& info) override
        {
            Call(FN_OnCaptureCpuProfilingStatisticsFinished, result, info);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<ProfilerNotificationBus>("ProfilerNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "profiler")
                    ->Handler<ProfilerNotificationBusHandler>();
            }
        }
    };

    bool SerializeCpuProfilingData(const AZStd::ring_buffer<CpuProfiler::TimeRegionMap>& data, AZStd::string outputFilePath, bool wasEnabled)
    {
        AZ_TracePrintf("ProfilerSystemComponent", "Beginning serialization of %zu frames of profiling data\n", data.size());
        AZ::JsonSerializerSettings serializationSettings;
        serializationSettings.m_keepDefaults = true;

        CpuProfilingStatisticsSerializer serializer(data);

        const auto saveResult = AZ::JsonSerializationUtils::SaveObjectToFile(&serializer,
            outputFilePath, (CpuProfilingStatisticsSerializer*)nullptr, &serializationSettings);

        AZStd::string captureInfo = outputFilePath;
        if (!saveResult.IsSuccess())
        {
            captureInfo = AZStd::string::format("Failed to save Cpu Profiling Statistics data to file '%s'. Error: %s",
                outputFilePath.c_str(),
                saveResult.GetError().c_str());
            AZ_Warning("ProfilerSystemComponent", false, captureInfo.c_str());
        }
        else
        {
            AZ_Printf("ProfilerSystemComponent", "Cpu profiling statistics was saved to file [%s]\n", outputFilePath.c_str());
        }

        // Disable the profiler again
        if (!wasEnabled)
        {
            CpuProfiler::Get()->SetProfilerEnabled(false);
        }

        // Notify listeners that the pass' PipelineStatistics queries capture has finished.
        ProfilerNotificationBus::Broadcast(&ProfilerNotificationBus::Events::OnCaptureCpuProfilingStatisticsFinished,
            saveResult.IsSuccess(),
            captureInfo);

        return saveResult.IsSuccess();
    }

    void ProfilerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ProfilerSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ProfilerSystemComponent>("Profiler", "Provides a custom implementation of the AZ::Debug::Profiler interface for capturing performance data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                ProfilerNotificationBusHandler::Reflect(context);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ProfilerRequestBus>("ProfilerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "profiler")
                ->Event("CaptureCpuProfilingStatistics", &ProfilerRequestBus::Events::CaptureCpuProfilingStatistics);

            ProfilerNotificationBusHandler::Reflect(context);
        }

        CpuProfilingStatisticsSerializer::Reflect(context);
    }

    void ProfilerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(profilerServiceCrc);
    }

    void ProfilerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(profilerServiceCrc);
    }

    void ProfilerSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ProfilerSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ProfilerSystemComponent::ProfilerSystemComponent()
    {
        if (ProfilerInterface::Get() == nullptr)
        {
            ProfilerInterface::Register(this);
        }
    }

    ProfilerSystemComponent::~ProfilerSystemComponent()
    {
        if (ProfilerInterface::Get() == this)
        {
            ProfilerInterface::Unregister(this);
        }
    }

    void ProfilerSystemComponent::Activate()
    {
        ProfilerRequestBus::Handler::BusConnect();

        m_cpuProfiler.Init();
    }

    void ProfilerSystemComponent::Deactivate()
    {
        m_cpuProfiler.Shutdown();

        ProfilerRequestBus::Handler::BusDisconnect();

        // Block deactivation until the IO thread has finished serializing the CPU data
        if (m_cpuDataSerializationThread.joinable())
        {
            m_cpuDataSerializationThread.join();
        }
    }

    void ProfilerSystemComponent::SetProfilerEnabled(bool enabled)
    {
        m_cpuProfiler.SetProfilerEnabled(enabled);
    }

    bool ProfilerSystemComponent::CaptureCpuProfilingStatistics(const AZStd::string& outputFilePath)
    {
        bool expected = false;
        if (!m_cpuCaptureInProgress.compare_exchange_strong(expected, true))
        {
            return false;
        }

        // Start the cpu profiling
        bool wasEnabled = m_cpuProfiler.IsProfilerEnabled();
        if (!wasEnabled)
        {
            m_cpuProfiler.SetProfilerEnabled(true);
        }

        const int frameDelay = 5; // arbitrary number
        DeplayedFunction delayedFunc(frameDelay,
            [this, outputFilePath, wasEnabled]()
            {
                // Blocking call for a single frame of data, avoid thread overhead
                AZStd::ring_buffer<CpuProfiler::TimeRegionMap> singleFrameData(1);
                singleFrameData.push_back(m_cpuProfiler.GetTimeRegionMap());
                SerializeCpuProfilingData(singleFrameData, outputFilePath, wasEnabled);
                m_cpuCaptureInProgress.store(false);
            }
        );
        delayedFunc.Run();

        return true;
    }

    bool ProfilerSystemComponent::BeginContinuousCpuProfilingCapture()
    {
        return m_cpuProfiler.BeginContinuousCapture();
    }

    bool ProfilerSystemComponent::EndContinuousCpuProfilingCapture(const AZStd::string& outputFilePath)
    {
        bool expected = false;
        if (!m_cpuDataSerializationInProgress.compare_exchange_strong(expected, true))
        {
            AZ_TracePrintf(
                "ProfilerSystemComponent",
                "Cannot end a continuous capture - another serialization is currently in progress\n");
            return false;
        }

        AZStd::ring_buffer<CpuProfiler::TimeRegionMap> captureResult;
        const bool captureEnded = m_cpuProfiler.EndContinuousCapture(captureResult);
        if (!captureEnded)
        {
            AZ_TracePrintf("ProfilerSystemComponent", "Could not end the continuous capture, is one in progress?\n");
            m_cpuDataSerializationInProgress.store(false);
            return false;
        }

        // cpuProfilingData could be 1GB+ once saved, so use an IO thread to write it to disk.
        auto threadIoFunction =
            [data = AZStd::move(captureResult), filePath = AZStd::string(outputFilePath), &flag = m_cpuDataSerializationInProgress]()
            {
                SerializeCpuProfilingData(data, filePath, true);
                flag.store(false);
            };

        // If the thread object already exists (ex. we have already serialized data), join. This will not block since
        // m_cpuDataSerializationInProgress was false, meaning the IO thread has already completed execution.
        if (m_cpuDataSerializationThread.joinable())
        {
            m_cpuDataSerializationThread.join();
        }

        auto thread = AZStd::thread(threadIoFunction);
        m_cpuDataSerializationThread = AZStd::move(thread);

        return true;
    }
} // namespace Profiler
