/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ::IO
{
    class FileDescriptorCapturer;
}

namespace AZ::SerializeContextTools
{
    class Application final
        : public AzToolsFramework::ToolsApplication
    {
    public:
        AZ_CLASS_ALLOCATOR(Application, AZ::SystemAllocator)
        Application(int argc, char** argv, AZ::IO::FileDescriptorCapturer* stdoutCapturer = {});
        Application(int argc, char** argv, ComponentApplicationSettings componentAppSettings);
        Application(int argc, char** argv, AZ::IO::FileDescriptorCapturer* stdoutCapturer, ComponentApplicationSettings componentAppSettings);
        ~Application() override = default;

        const char* GetConfigFilePath() const;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;

        //! Associates a FileDescriptorCapturer with the SerializeContextApplication that
        //! redirects stdout to a visitor callback.
        //! The FileDescriptorCapturer supports a write bypass to force writing to stdout if need be
        void SetStdoutCapturer(AZ::IO::FileDescriptorCapturer* stdoutCapturer);
        AZ::IO::FileDescriptorCapturer* GetStdoutCapturer() const;

    protected:
        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;

        AZ::IO::FixedMaxPath m_configFilePath;
        AZ::IO::FileDescriptorCapturer* m_stdoutCapturer;
    };
} // namespace AZ::SerializeContextTools
