/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NullArchiveComponent.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{

    void NullArchiveComponent::Activate()
    {
        ArchiveCommandsBus::Handler::BusConnect();
    }

    void NullArchiveComponent::Deactivate()
    {
        ArchiveCommandsBus::Handler::BusDisconnect();
    }

    std::future<bool> DefaultFuture()
    {
        std::promise<bool> p;
        p.set_value(false);
        return p.get_future();
    }

    std::future<bool> NullArchiveComponent::CreateArchive(
        const AZStd::string& /*archivePath*/,
        const AZStd::string& /*dirToArchive*/)
    {
        return DefaultFuture();
    }

    std::future<bool> NullArchiveComponent::ExtractArchive(
        const AZStd::string& /*archivePath*/,
        const AZStd::string& /*destinationPath*/)
    {
        return DefaultFuture();
    }

    std::future<bool> NullArchiveComponent::ExtractFile(
        const AZStd::string& /*archivePath*/,
        const AZStd::string& /*fileInArchive*/,
        const AZStd::string& /*destinationPath*/)
    {
        return DefaultFuture();
    }

    bool NullArchiveComponent::ListFilesInArchive(const AZStd::string& /*archivePath*/, AZStd::vector<AZStd::string>& /*outFileEntries*/)
    {
        return false;
    }

    std::future<bool> NullArchiveComponent::AddFileToArchive(
        const AZStd::string& /*archivePath*/,
        const AZStd::string& /*fileToAdd*/,
        const AZStd::string& /*pathInArchive*/)
    {
        return DefaultFuture();
    }

    std::future<bool> NullArchiveComponent::AddFilesToArchive(
        const AZStd::string& /*archivePath*/,
        const AZStd::string& /*workingDirectory*/,
        const AZStd::string& /*listFilePath*/)
    {
        return DefaultFuture();
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
