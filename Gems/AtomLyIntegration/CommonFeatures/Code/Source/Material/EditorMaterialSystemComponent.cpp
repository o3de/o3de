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

#include <Material/EditorMaterialSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Application/Application.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>

#include <AtomToolsFramework/Util/Util.h>

#include <Material/MaterialThumbnail.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QApplication>
#include <QProcessEnvironment>
#include <QObject>
#include <QAction>
AZ_POP_DISABLE_WARNING

void InitMaterialEditorResources()
{
    //Must register qt resources from other modules
    Q_INIT_RESOURCE(InspectorWidget);
}

namespace AZ
{
    namespace Render
    {
        //! Main system component for the Atom Common Feature Gem's editor/tools module.
        void EditorMaterialSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorMaterialSystemComponent, AZ::Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorMaterialSystemComponent>("EditorMaterialSystemComponent", "System component that manages launching and maintaining connections the material editor.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void EditorMaterialSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EditorMaterialSystem", 0x5c93bc4e));
        }

        void EditorMaterialSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EditorMaterialSystem", 0x5c93bc4e));
        }

        void EditorMaterialSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("ThumbnailerService", 0x65422b97));
        }

        void EditorMaterialSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void EditorMaterialSystemComponent::Init()
        {
            InitMaterialEditorResources();
        }

        void EditorMaterialSystemComponent::Activate()
        {
            AzFramework::TargetManagerClient::Bus::Handler::BusConnect();
            EditorMaterialSystemComponentRequestBus::Handler::BusConnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
            AzToolsFramework::EditorMenuNotificationBus::Handler::BusConnect();

            SetupThumbnails();
        }

        void EditorMaterialSystemComponent::Deactivate()
        {
            AzFramework::TargetManagerClient::Bus::Handler::BusDisconnect();
            EditorMaterialSystemComponentRequestBus::Handler::BusDisconnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorMenuNotificationBus::Handler::BusDisconnect();

            TeardownThumbnails();

            if (m_openMaterialEditorAction)
            {
                delete m_openMaterialEditorAction;
                m_openMaterialEditorAction = nullptr;
            }
        }

        void EditorMaterialSystemComponent::OpenInMaterialEditor(const AZStd::string& sourcePath)
        {
            if (m_materialEditorTarget.IsValid())
            {
                AzFramework::TmMsg openDocumentMsg(AZ_CRC("OpenInMaterialEditor", 0x9f92aac8));
                openDocumentMsg.AddCustomBlob(sourcePath.c_str(), sourcePath.size() + 1);
                AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::SendTmMessage, m_materialEditorTarget, openDocumentMsg);
            }
            else
            {
                AZ_TracePrintf("MaterialComponent", "Launching Material Editor");

                QStringList arguments;
                    arguments.append(sourcePath.c_str());
                AtomToolsFramework::LaunchTool("MaterialEditor", ".exe", arguments);
            }
        }

        void EditorMaterialSystemComponent::TargetJoinedNetwork(AzFramework::TargetInfo info)
        {
            if (AZ::StringFunc::Equal(info.GetDisplayName(), "MaterialEditor"))
            {
                m_materialEditorTarget = info;
            }
        }

        void EditorMaterialSystemComponent::TargetLeftNetwork(AzFramework::TargetInfo info)
        {
            if (AZ::StringFunc::Equal(info.GetDisplayName(), "MaterialEditor"))
            {
                m_materialEditorTarget = {};
            }
        }

        void EditorMaterialSystemComponent::OnApplicationAboutToStop()
        {
            TeardownThumbnails();
        }
        
        void EditorMaterialSystemComponent::OnPopulateToolMenuItems()
        {
            if (!m_openMaterialEditorAction)
            {
                m_openMaterialEditorAction = new QAction("Material Editor");
                m_openMaterialEditorAction->setShortcut(QKeySequence(Qt::Key_M));
                QObject::connect(m_openMaterialEditorAction, &QAction::triggered, m_openMaterialEditorAction, [this]()
                    {
                        OpenInMaterialEditor("");
                    }
                );

                AzToolsFramework::EditorMenuRequestBus::Broadcast(&AzToolsFramework::EditorMenuRequestBus::Handler::AddMenuAction, "ToolMenu", m_openMaterialEditorAction);
            }
        }

        void EditorMaterialSystemComponent::OnResetToolMenuItems()
        {
            if (m_openMaterialEditorAction)
            {
                delete m_openMaterialEditorAction;
                m_openMaterialEditorAction = nullptr;
            }
        }

        void EditorMaterialSystemComponent::SetupThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;
            using namespace LyIntegration;

            ThumbnailerRequestsBus::Broadcast(&ThumbnailerRequests::RegisterThumbnailProvider,
                MAKE_TCACHE(Thumbnails::MaterialThumbnailCache),
                ThumbnailContext::DefaultContext);
        }

        void EditorMaterialSystemComponent::TeardownThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;
            using namespace LyIntegration;

            ThumbnailerRequestsBus::Broadcast(&ThumbnailerRequests::UnregisterThumbnailProvider,
                Thumbnails::MaterialThumbnailCache::ProviderName,
                ThumbnailContext::DefaultContext);
        }

        AzToolsFramework::AssetBrowser::SourceFileDetails EditorMaterialSystemComponent::GetSourceFileDetails(const char* fullSourceFileName)
        {
            static const char* MaterialTypeIconPath = ":/Icons/materialtype.svg";
            static const char* MaterialTypeExtension = "materialtype";
            if (AzFramework::StringFunc::EndsWith(fullSourceFileName, MaterialTypeExtension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(MaterialTypeIconPath);
            }
            return AzToolsFramework::AssetBrowser::SourceFileDetails();
        }
    } // namespace Render
} // namespace AZ
