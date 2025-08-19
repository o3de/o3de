/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/wildcard.h>
#include <Editor/ParticleBrowserInteractions.h>
#include <OpenParticleSystem/EditorParticleSystemComponentRequestBus.h>

namespace OpenParticle
{
    ParticleBrowserInteractions::ParticleBrowserInteractions()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }

    ParticleBrowserInteractions::~ParticleBrowserInteractions()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    }

    void ParticleBrowserInteractions::AddSourceFileOpeners(
        const char* fullSourceFileName,
        [[maybe_unused]] const AZ::Uuid& sourceUUID,
        AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        if (HandlesSource(fullSourceFileName))
        {
            openers.push_back(
                {
                    "Particle_Editor",
                    QObject::tr("Open in Particle Editor...").toUtf8().data(),
                    QIcon(),
                    [&]([[maybe_unused]] const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorParticleSystemComponentRequestBus::Broadcast(&EditorParticleSystemComponentRequestBus::Events::OpenParticleEditor, fullSourceFileNameInCallback);
                    }
                });
        }
    }

    bool ParticleBrowserInteractions::HandlesSource(AZStd::string_view fileName) const
    {
        return AZStd::wildcard_match("*.particle", fileName.data());
    }
} // namespace OpenParticle
