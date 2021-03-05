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

#include "NullArchiveComponent.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{

    void NullArchiveComponent::Activate()
    {
        ArchiveCommands::Bus::Handler::BusConnect();
    }

    void NullArchiveComponent::Deactivate()
    {
        ArchiveCommands::Bus::Handler::BusDisconnect();
    }

    bool NullArchiveComponent::ExtractArchiveBlocking(const AZStd::string& /*archivePath*/, const AZStd::string& /*destinationPath*/, bool /*extractWithRootDirectory*/)
    {
        return false;
    }

    void NullArchiveComponent::ExtractArchive(const AZStd::string& /*archivePath*/, const AZStd::string& /*destinationPath*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseCallback& respCallback)
    {
        AZ::TickBus::QueueFunction(respCallback, false);
    }

    void NullArchiveComponent::ExtractArchiveOutput(const AZStd::string& /*archivePath*/, const AZStd::string& /*destinationPath*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseOutputCallback& respCallback)
    {
        AZ::TickBus::QueueFunction(respCallback, false, AZStd::string());
    }

    void NullArchiveComponent::ExtractArchiveWithoutRoot(const AZStd::string& /*archivePath*/, const AZStd::string& /*destinationPath*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseOutputCallback& respCallback)
    {
        AZ::TickBus::QueueFunction(respCallback, false, AZStd::string());
    }

    void NullArchiveComponent::ExtractFile(const AZStd::string& /*archivePath*/, const AZStd::string& /*fileInArchive*/, const AZStd::string& /*destinationPath*/, bool /*overWrite*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseOutputCallback& respCallback)
    {
        // Always report we failed to extract
        AZ::TickBus::QueueFunction(respCallback, false, AZStd::string());
    }

    bool NullArchiveComponent::ExtractFileBlocking(const AZStd::string& /*archivePath*/, const AZStd::string& /*fileInArchive*/, const AZStd::string& /*destinationPath*/, bool /*overWrite*/)
    {
        return false;
    }

    void NullArchiveComponent::ListFilesInArchive(const AZStd::string& /*archivePath*/, AZStd::vector<AZStd::string>& /*consoleOutput*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseOutputCallback& respCallback)
    {
        // Always report we failed to extract
        AZ::TickBus::QueueFunction(respCallback, false, AZStd::string());
    }

    bool NullArchiveComponent::ListFilesInArchiveBlocking(const AZStd::string& /*archivePath*/, AZStd::vector<AZStd::string>& /*consoleOutput*/)
    {
        return false;
    }

    void NullArchiveComponent::AddFileToArchive(const AZStd::string& /*archivePath*/, const AZStd::string& /*fileToAdd*/, const AZStd::string& /*pathInArchive*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseOutputCallback& respCallback)
    {
        // Always report we failed to extract
        AZ::TickBus::QueueFunction(respCallback, false, AZStd::string());
    }

    bool NullArchiveComponent::AddFileToArchiveBlocking(const AZStd::string& /*archivePath*/, const AZStd::string& /*fileToAdd*/, const AZStd::string& /*pathInArchive*/)
    {
        return false;
    }

    bool NullArchiveComponent::AddFilesToArchiveBlocking(const AZStd::string& /*archivePath*/, const AZStd::string& /*workingDirectory*/, const AZStd::string& /*listFilePath*/)
    {
        return false;
    }

    void NullArchiveComponent::AddFilesToArchive(const AZStd::string& /*archivePath*/, const AZStd::string& /*workingDirectory*/, const AZStd::string& /*listFilePath*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseOutputCallback& respCallback)
    {
        // Always report we failed to extract
        AZ::TickBus::QueueFunction(respCallback, false, AZStd::string());
    }

    void NullArchiveComponent::CreateArchive(const AZStd::string& /*archivePath*/, const AZStd::string& /*dirToArchive*/, AZ::Uuid /*taskHandle*/, const ArchiveResponseOutputCallback& respCallback)
    {
        // Always report we failed to extract
        AZ::TickBus::QueueFunction(respCallback, false, AZStd::string());
    }

    bool NullArchiveComponent::CreateArchiveBlocking(const AZStd::string& /*archivePath*/, const AZStd::string& /*dirToArchive*/)
    {
        return false;
    }

    void NullArchiveComponent::CancelTasks(AZ::Uuid /*taskHandle*/)
    {
    }

    void NullArchiveComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<NullArchiveComponent, AZ::Component>()
                ;
        }
    }
} // namespace AzToolsFramework
