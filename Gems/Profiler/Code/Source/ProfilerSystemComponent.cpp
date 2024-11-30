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

    struct DelayedFunction
    {
        using func_type = AZStd::function<void()>;

        DelayedFunction(int framesToDelay, func_type&& function)
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
                    [](DelayedFunction&& delayedFunc)
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

    bool SerializeCpuProfilingData(const AZStd::ring_buffer<TimeRegionMap>& data, AZStd::string outputFilePath, bool wasEnabled)
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
            AZ::Debug::ProfilerSystemInterface::Get()->SetActive(false);
        }

        // Notify listeners that the profiler capture has finished.
        AZ::Debug::ProfilerNotificationBus::Broadcast(&AZ::Debug::ProfilerNotificationBus::Events::OnCaptureFinished,
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
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
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
        if (AZ::Debug::ProfilerSystemInterface::Get() == nullptr)
        {
            AZ::Debug::ProfilerSystemInterface::Register(this);
        }
    }

    ProfilerSystemComponent::~ProfilerSystemComponent()
    {
        if (AZ::Debug::ProfilerSystemInterface::Get() == this)
        {
            AZ::Debug::ProfilerSystemInterface::Unregister(this);
        }
    }

    void ProfilerSystemComponent::Activate()
    {
        m_cpuProfiler.Init();
    }

    void ProfilerSystemComponent::Deactivate()
    {
        m_cpuProfiler.Shutdown();

        // Block deactivation until the IO thread has finished serializing the CPU data
        if (m_cpuDataSerializationThread.joinable())
        {
            m_cpuDataSerializationThread.join();
        }
    }

    bool ProfilerSystemComponent::IsActive() const
    {
        return m_cpuProfiler.IsProfilerEnabled();
    }

    void ProfilerSystemComponent::SetActive(bool enabled)
    {
        m_cpuProfiler.SetProfilerEnabled(enabled);
    }

    bool ProfilerSystemComponent::CaptureFrame(const AZStd::string& outputFilePath)
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
        DelayedFunction delayedFunc(frameDelay,
            [this, outputFilePath, wasEnabled]()
            {
                // Blocking call for a single frame of data, avoid thread overhead
                AZStd::ring_buffer<TimeRegionMap> singleFrameData(1);
                singleFrameData.push_back(m_cpuProfiler.GetTimeRegionMap());
                SerializeCpuProfilingData(singleFrameData, outputFilePath, wasEnabled);
                m_cpuCaptureInProgress.store(false);
            }
        );
        delayedFunc.Run();

        return true;
    }

    bool ProfilerSystemComponent::StartCapture(AZStd::string outputFilePath)
    {
        m_captureFile = AZStd::move(outputFilePath);
        return m_cpuProfiler.BeginContinuousCapture();
    }

    bool ProfilerSystemComponent::EndCapture()
    {
        bool expected = false;
        if (!m_cpuDataSerializationInProgress.compare_exchange_strong(expected, true))
        {
            AZ_TracePrintf(
                "ProfilerSystemComponent",
                "Cannot end a continuous capture - another serialization is currently in progress\n");
            return false;
        }

        AZStd::ring_buffer<TimeRegionMap> captureResult;
        const bool captureEnded = m_cpuProfiler.EndContinuousCapture(captureResult);
        if (!captureEnded)
        {
            AZ_TracePrintf("ProfilerSystemComponent", "Could not end the continuous capture, is one in progress?\n");
            m_cpuDataSerializationInProgress.store(false);
            return false;
        }

        // cpuProfilingData could be 1GB+ once saved, so use an IO thread to write it to disk.
        auto threadIoFunction =
            [data = AZStd::move(captureResult), filePath = m_captureFile, &flag = m_cpuDataSerializationInProgress]()
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

    bool ProfilerSystemComponent::IsCaptureInProgress() const
    {
        return m_cpuProfiler.IsContinuousCaptureInProgress();
    }
} // namespace Profiler
