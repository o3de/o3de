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

#include <AzCore/std/string/wildcard.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <Material/MaterialBrowserInteractions.h>

namespace AZ
{
    namespace Render
    {
        MaterialBrowserInteractions::MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        }

        MaterialBrowserInteractions::~MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        }

        void MaterialBrowserInteractions::AddSourceFileOpeners(const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
        {
            if (HandlesSource(fullSourceFileName))
            {
                openers.push_back(
                    {
                        "Material_Editor",
                        "Open in Material Editor...",
                        QIcon(),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                        {
                            EditorMaterialSystemComponentRequestBus::Broadcast(&EditorMaterialSystemComponentRequestBus::Events::OpenInMaterialEditor, fullSourceFileNameInCallback);
                        }
                    });
            }
        }

        bool MaterialBrowserInteractions::HandlesSource(AZStd::string_view fileName) const
        {
            return AZStd::wildcard_match("*.material", fileName.data());
        }
    } // namespace Render
} // namespace AZ
