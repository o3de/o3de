/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <Editor/LyViewPaneNames.h>
#include <Material/EditorMaterialComponentInspector.h>
#include <Material/EditorMaterialSystemComponent.h>
#include <Material/MaterialThumbnail.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QObject>
#include <QProcessEnvironment>
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
            EditorMaterialSystemComponentRequestBus::Handler::BusConnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
            AzToolsFramework::EditorMenuNotificationBus::Handler::BusConnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

            SetupThumbnails();
            m_materialBrowserInteractions.reset(aznew MaterialBrowserInteractions);
        }

        void EditorMaterialSystemComponent::Deactivate()
        {
            EditorMaterialSystemComponentRequestBus::Handler::BusDisconnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorMenuNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect(); 

            TeardownThumbnails();
            m_materialBrowserInteractions.reset();

            if (m_openMaterialEditorAction)
            {
                delete m_openMaterialEditorAction;
                m_openMaterialEditorAction = nullptr;
            }
        }

        void EditorMaterialSystemComponent::OpenMaterialEditor(const AZStd::string& sourcePath)
        {
            AZ_TracePrintf("MaterialComponent", "Launching Material Editor");

            QStringList arguments;
            arguments.append(sourcePath.c_str());

            // Use the same RHI as the main editor
            AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
            if (!apiName.IsEmpty())
            {
                arguments.append(QString("--rhi=%1").arg(apiName.GetCStr()));
            }

            AZ::IO::FixedMaxPathString projectPath(AZ::Utils::GetProjectPath());
            if (!projectPath.empty())
            {
                arguments.append(QString("--project-path=%1").arg(projectPath.c_str()));
            }

            AtomToolsFramework::LaunchTool("MaterialEditor", ".exe", arguments);
        }

        void EditorMaterialSystemComponent::OpenMaterialInspector(
            const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId)
        {
            auto dockWidget = AzToolsFramework::InstanceViewPane("Material Property Inspector");
            if (dockWidget)
            {
                auto inspector = static_cast<AZ::Render::EditorMaterialComponentInspector::MaterialPropertyInspector*>(dockWidget->widget());
                if (inspector)
                {
                    inspector->LoadMaterial(entityId, materialAssignmentId);
                }
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
                m_openMaterialEditorAction->setCheckable(false);
                m_openMaterialEditorAction->setChecked(false);
                m_openMaterialEditorAction->setIcon(QIcon(":/Menu/material_editor.svg"));
                QObject::connect(
                    m_openMaterialEditorAction, &QAction::triggered, m_openMaterialEditorAction, [this]()
                    {
                        OpenMaterialEditor("");
                    }
                );

                AzToolsFramework::EditorMenuRequestBus::Broadcast(
                    &AzToolsFramework::EditorMenuRequestBus::Handler::AddMenuAction, "ToolMenu", m_openMaterialEditorAction, true);
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

        void EditorMaterialSystemComponent::NotifyRegisterViews()
        {
            AzToolsFramework::ViewPaneOptions inspectorOptions;
            inspectorOptions.canHaveMultipleInstances = true;
            inspectorOptions.preferedDockingArea = Qt::NoDockWidgetArea;
            inspectorOptions.paneRect = QRect(50, 50, 400, 700);
            inspectorOptions.showInMenu = false;
            inspectorOptions.showOnToolsToolbar = false;
            AzToolsFramework::RegisterViewPane<AZ::Render::EditorMaterialComponentInspector::MaterialPropertyInspector>(
                "Material Property Inspector", LyViewPane::CategoryTools, inspectorOptions);
        }

        void EditorMaterialSystemComponent::SetupThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;
            using namespace LyIntegration;

            ThumbnailerRequestsBus::Broadcast(
                &ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(Thumbnails::MaterialThumbnailCache),
                ThumbnailContext::DefaultContext);
        }

        void EditorMaterialSystemComponent::TeardownThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;
            using namespace LyIntegration;

            ThumbnailerRequestsBus::Broadcast(
                &ThumbnailerRequests::UnregisterThumbnailProvider, Thumbnails::MaterialThumbnailCache::ProviderName,
                ThumbnailContext::DefaultContext);
        }

        AzToolsFramework::AssetBrowser::SourceFileDetails EditorMaterialSystemComponent::GetSourceFileDetails(
            const char* fullSourceFileName)
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
