/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleBrowserInteractions.h"

#include <AzCore/std/string/wildcard.h>
#include <Editor/EditorParticleSystemComponentRequestBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include "AnimationContext.h"

#include <QMenu>
#include <QInputDialog>
#include <QString>

namespace OpenParticleSystemEditor {
    class ParticleDocument;
}

#include <QObject>
#include <QIcon>

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

    void ParticleBrowserInteractions::AddSourceFileCreators([[maybe_unused]] const char *fullSourceFolderName,
                                                            [[maybe_unused]] const AZ::Uuid &sourceUUID,
                                                            AzToolsFramework::AssetBrowser::SourceFileCreatorList &creators)
    {
        creators.push_back({    "Particle_Creator",
                                "Particle System",
                                QIcon(),
                                [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
        {
            bool ok;
            const QString fileName =QInputDialog::getText(nullptr,
                                                          QObject::tr("Create New Particle"),
                                                          QObject::tr("Particle name:"),
                                                          QLineEdit::Normal,
                                                          "MyParticle",
                                                          &ok);
            if (!fileName.isEmpty())
            {
                AZStd::string fullFilepath;
                AZ::StringFunc::Path::ConstructFull(fullSourceFolderNameInCallback.c_str(),
                                                    fileName.toUtf8(),
                                                    ".particle",
                                                    fullFilepath);

                EditorParticleSystemComponentRequestBus::Broadcast(&EditorParticleSystemComponentRequestBus::Events::CreateNewParticle,
                                                                    fullFilepath);
            }

        }});
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
                    EditorParticleSystemComponentRequestBus::Broadcast(&EditorParticleSystemComponentRequestBus::Events::OpenParticleEditor,
                        fullSourceFileNameInCallback);
                }
            });
        }
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails ParticleBrowserInteractions::GetSourceFileDetails(const char* fullSourceFileName)
    {
        const AZStd::string_view path(fullSourceFileName);
        if (path.ends_with("particle"))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails(":/OpenParticleSystem/Icons/Particle.svg");
        }

        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    bool ParticleBrowserInteractions::HandlesSource(AZStd::string_view fileName) const
    {
        return AZStd::wildcard_match("*.particle", fileName.data());
    }

} // namespace OpenParticle
