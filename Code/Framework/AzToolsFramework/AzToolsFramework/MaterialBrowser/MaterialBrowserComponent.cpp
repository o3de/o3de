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
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/MaterialBrowser/MaterialBrowserComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/FolderThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/MaterialBrowser/MaterialThumbnail.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>

#include <QApplication>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
#include <QStyle>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace MaterialBrowser
    {
        MaterialBrowserComponent::MaterialBrowserComponent()
        {
        }

        void MaterialBrowserComponent::Activate()
        {
            using namespace Thumbnailer;
            using namespace AssetBrowser;
            const char* contextName = "MaterialBrowser";
            ThumbnailerRequestsBus::Broadcast(&ThumbnailerRequests::RegisterContext, contextName);
            ThumbnailerRequestsBus::Broadcast(&ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(FolderThumbnailCache), contextName);
            ThumbnailerRequestsBus::Broadcast(&ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(SourceThumbnailCache), contextName);
            ThumbnailerRequestsBus::Broadcast(&ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(MaterialThumbnailCache), contextName);
            ThumbnailerRequestsBus::Broadcast(&ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(SourceControlThumbnailCache), contextName);
        }

        void MaterialBrowserComponent::Deactivate()
        {
        }

        void MaterialBrowserComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<MaterialBrowserComponent, AZ::Component>();
            }
        }

        void MaterialBrowserComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("ThumbnailerService", 0x65422b97));
        }
    } // namespace MaterialBrowser
} // namespace AzToolsFramework

