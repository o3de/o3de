/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI.Profiler/PIX/PIXSystemComponent.h>
#include <RHI.Profiler/Utils.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Atom_RHI_Traits_Platform.h>

#include <WinPixEventRuntime/pix3.h>

namespace AZ::RHI
{
    namespace Internal
    {
        using FixedMaxPathWString = AZStd::fixed_wstring<AZ::IO::MaxPathLength>;
        AZ::IO::FixedMaxPathString GetLatestWinPixGpuCapturerPath();
        auto GetCaptureFolderPath() -> AZ::IO::FixedMaxPathString
        {
            auto fileIO = AZ::IO::FileIOBase::GetInstance();
            constexpr const char* capturePath = "@user@/PIX";
            AZ::IO::FixedMaxPath resolvedPath;
            fileIO->ResolvePath(resolvedPath, capturePath);
            return AZ::IO::FixedMaxPathString(AZStd::move(resolvedPath));
        }

        auto GenerateCaptureName() -> FixedMaxPathWString
        {
            AZStd::fixed_string<128> sTime;
            constexpr const char* timeFormat = "%Y%m%d_%H%M%S";
#ifdef AZ_COMPILER_MSVC
            time_t ltime;
            time(&ltime);
            struct tm today;
            localtime_s(&today, &ltime);
            strftime(sTime.data(), sTime.capacity(), timeFormat, &today);
            // Fix up the internal size member of the fixed string
            // Using the traits_type::length function to calculate the strlen of the c-string returned by strftime
            sTime.resize_no_construct(decltype(sTime)::traits_type::length(sTime.data()));
#else
            time_t ltime;
            time(&ltime);
            auto today = localtime(&ltime);
            strftime(sTime.data(), sTime.capacity(), timeFormat, today);
            sTime.resize_no_construct(decltype(sTime)::traits_type::length(sTime.data()));
#endif
            auto fileIO = AZ::IO::FileIOBase::GetInstance();
            auto captureFolderPath = GetCaptureFolderPath();
            fileIO->CreatePath(captureFolderPath.c_str());
            AZStd::string capturePath;
            AzFramework::StringFunc::Path::ConstructFull(captureFolderPath.c_str(), sTime.c_str(), "wpix", capturePath);  
            FixedMaxPathWString capturePathW;
            AZStd::to_wstring(capturePathW, capturePath.c_str());
            return capturePathW;
        };
    } // namespace Internal

    void PIXSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<PIXSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void PIXSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GraphicsProfilerService"));
    }

    void PIXSystemComponent::Activate()
    {
        bool loadPIX = RHI::ShouldLoadProfiler("PIX");
        // Get the path to the latest pix install directory
        m_dynamicModule = DynamicModuleHandle::Create(Internal::GetLatestWinPixGpuCapturerPath().c_str());
        AZ_Assert(m_dynamicModule, "Failed to create PIX dynamic module");
        if (m_dynamicModule->Load(loadPIX ? DynamicModuleHandle::LoadFlags::None : DynamicModuleHandle::LoadFlags::NoLoad))
        {
            GraphicsProfilerBus::Handler::BusConnect();
            AZ_Printf("PIXSystemComponent", "PIX profiler connected. Capture path is %s.\n", Internal::GetCaptureFolderPath().c_str());
        }
    }

    void PIXSystemComponent::Deactivate()
    {
        GraphicsProfilerBus::Handler::BusDisconnect();
        if (m_dynamicModule)
        {
            m_dynamicModule->Unload();
        }
    }

    void PIXSystemComponent::StartCapture(const AzFramework::NativeWindowHandle window)
    {
        auto filePath = Internal::GenerateCaptureName();
        PIXCaptureParameters params{};
        params.GpuCaptureParameters.FileName = filePath.c_str();
        PIXSetTargetWindow(reinterpret_cast<HWND>(window));
        PIXBeginCapture(PIX_CAPTURE_GPU, &params);
    }

    bool PIXSystemComponent::EndCapture([[maybe_unused]] const AzFramework::NativeWindowHandle window)
    {
        HRESULT hRes = PIXEndCapture(false);
        return !FAILED(hRes);
    }

    void PIXSystemComponent::TriggerCapture()
    {
        auto filePath = Internal::GenerateCaptureName();
        PIXGpuCaptureNextFrames(filePath.c_str(), 1);
        AZ_Printf("PIXSystemComponent", "Saving PIX capture to %s\n", filePath.c_str());
    }
} // namespace AZ::RHI
