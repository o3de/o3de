/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentNotificationBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/AtomImGuiTools/AtomImGuiToolsBus.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRendererCaptureRequest.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRendererInterface.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManager.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <Editor/LyViewPaneNames.h>
#include <Material/EditorMaterialComponentInspector.h>
#include <Material/EditorMaterialSystemComponent.h>
#include <SharedPreview/SharedPreviewContent.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QProcessEnvironment>
AZ_POP_DISABLE_WARNING

constexpr AZStd::string_view MaterialCanvasActionIdentifier = "o3de.action.tools.material_canvas";
constexpr AZStd::string_view MaterialEditorActionIdentifier = "o3de.action.tools.material_editor";

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
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void EditorMaterialSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EditorMaterialSystem"));
        }

        void EditorMaterialSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EditorMaterialSystem"));
        }

        void EditorMaterialSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PreviewRendererSystem"));
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
            AZ::EntitySystemBus::Handler::BusConnect();
            EditorMaterialSystemComponentNotificationBus::Handler::BusConnect();
            EditorMaterialSystemComponentRequestBus::Handler::BusConnect();
            MaterialComponentNotificationBus::Router::BusRouterConnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
            AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();

            // All material previews use the same model and lighting preset assets
            // models/sphere.azmodel
            m_materialPreviewModelAsset.Create(AZ::Data::AssetId("{6DE0E9A8-A1C7-5D0F-9407-4E627C1F223C}", 284780167));
            // lightingpresets/thumbnail.lightingpreset.azasset
            m_materialPreviewLightingPresetAsset.Create(AZ::Data::AssetId("{4F3761EF-E279-5FDD-98C3-EF90F924FBAC}", 0));

            m_materialBrowserInteractions.reset(aznew MaterialBrowserInteractions);
        }

        void EditorMaterialSystemComponent::Deactivate()
        {
            AZ::EntitySystemBus::Handler::BusDisconnect();
            EditorMaterialSystemComponentNotificationBus::Handler::BusDisconnect();
            EditorMaterialSystemComponentRequestBus::Handler::BusDisconnect();
            MaterialComponentNotificationBus::Router::BusRouterDisconnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect(); 
            AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect(); 
            AZ::SystemTickBus::Handler::BusDisconnect();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

            m_materialBrowserInteractions.reset();
            m_materialPreviewRequests.clear();
            m_materialPreviewModelAsset.Release();
            m_materialPreviewLightingPresetAsset.Release();
        }

        void EditorMaterialSystemComponent::OpenMaterialEditor(const AZStd::string& sourcePath)
        {
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

            AZ_TracePrintf("MaterialComponent", "Launching Material Editor");
            AtomToolsFramework::LaunchTool("MaterialEditor", arguments);
        }

        void EditorMaterialSystemComponent::OpenMaterialCanvas(const AZStd::string& sourcePath)
        {
            QStringList arguments;
            arguments.append(sourcePath.c_str());

            // Use the same RHI as the main Canvas
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

            AZ_TracePrintf("MaterialComponent", "Launching Material Canvas");
            AtomToolsFramework::LaunchTool("MaterialCanvas", arguments);
        }

        void EditorMaterialSystemComponent::OpenMaterialInspector(
            const AZ::EntityId& primaryEntityId,
            const AzToolsFramework::EntityIdSet& entityIdsToEdit,
            const AZ::Render::MaterialAssignmentId& materialAssignmentId)
        {
            auto dockWidget = AzToolsFramework::InstanceViewPane("Material Instance Editor");
            if (dockWidget)
            {
                auto inspector = static_cast<AZ::Render::EditorMaterialComponentInspector::MaterialPropertyInspector*>(dockWidget->widget());
                if (inspector)
                {
                    inspector->LoadMaterial(primaryEntityId, entityIdsToEdit, materialAssignmentId);
                }
            }
        }

        void EditorMaterialSystemComponent::RenderMaterialPreview(
            const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId)
        {
            m_materialPreviewRequests.emplace(entityId, materialAssignmentId);
            AZ::SystemTickBus::Handler::BusConnect();
        }

        QPixmap EditorMaterialSystemComponent::GetRenderedMaterialPreview(
            const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) const
        {
            const auto& itr1 = m_materialPreviews.find(entityId);
            if (itr1 != m_materialPreviews.end())
            {
                const auto& itr2 = itr1->second.find(materialAssignmentId);
                if (itr2 != itr1->second.end())
                {
                    return itr2->second;
                }
            }
            return QPixmap();
        }

        void EditorMaterialSystemComponent::OnEntityDeactivated(const AZ::EntityId& entityId)
        {
            // Deleting any preview saved for an entity that is about to be deactivated
            m_materialPreviews.erase(entityId);
        }

        void EditorMaterialSystemComponent::OnSystemTick()
        {
            auto previewRenderer = AZ::Interface<AtomToolsFramework::PreviewRendererInterface>::Get();
            if (!previewRenderer || !m_materialPreviewModelAsset.IsReady() || !m_materialPreviewLightingPresetAsset.IsReady())
            {
                return;
            }

            for (const auto& materialPreviewRequestPair : m_materialPreviewRequests)
            {
                const auto& entityId = materialPreviewRequestPair.first;
                const auto& materialAssignmentId = materialPreviewRequestPair.second;

                AZ::Data::AssetId materialAssetId = {};
                MaterialComponentRequestBus::EventResult(
                    materialAssetId, entityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId, materialAssignmentId);

                AZ::Render::MaterialPropertyOverrideMap propertyOverrides;
                AZ::Render::MaterialComponentRequestBus::EventResult(
                    propertyOverrides, entityId, &AZ::Render::MaterialComponentRequestBus::Events::GetPropertyValues, materialAssignmentId);

                // Having an invalid material asset will use the default asset on the model.
                AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;
                materialAsset.Create(materialAssetId);

                previewRenderer->AddCaptureRequest(
                    { MaterialPreviewResolution,
                      AZStd::make_shared<AZ::LyIntegration::SharedPreviewContent>(
                          previewRenderer->GetScene(),
                          previewRenderer->GetView(),
                          previewRenderer->GetEntityContextId(),
                          m_materialPreviewModelAsset,
                          materialAsset,
                          m_materialPreviewLightingPresetAsset,
                          propertyOverrides),
                      [entityId, materialAssignmentId]()
                      {
                          AZ_UNUSED(entityId);
                          AZ_UNUSED(materialAssignmentId);

                          AZ_Warning(
                              "EditorMaterialSystemComponent",
                              false,
                              "RenderMaterialPreview capture failed for entity %s slot %s.",
                              entityId.ToString().c_str(),
                              materialAssignmentId.ToString().c_str());

                          // If the capture failed to render substitute it with a black image
                          QPixmap pixmap(1, 1);
                          pixmap.fill(Qt::black);
                          AZ::Render::EditorMaterialSystemComponentNotificationBus::Broadcast(
                              &AZ::Render::EditorMaterialSystemComponentNotificationBus::Events::OnRenderMaterialPreviewRendered,
                              entityId,
                              materialAssignmentId,
                              pixmap);
                      },
                      [entityId, materialAssignmentId](const QPixmap& pixmap)
                      {
                          AZ::Render::EditorMaterialSystemComponentNotificationBus::Broadcast(
                              &AZ::Render::EditorMaterialSystemComponentNotificationBus::Events::OnRenderMaterialPreviewRendered,
                              entityId,
                              materialAssignmentId,
                              pixmap);
                      } });
            }

            m_materialPreviewRequests.clear();
            AZ::SystemTickBus::Handler::BusDisconnect();
        }

        void EditorMaterialSystemComponent::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
        {
            m_materialPreviewModelAsset.QueueLoad();
            m_materialPreviewLightingPresetAsset.QueueLoad();
        }

        void EditorMaterialSystemComponent::OnRenderMaterialPreviewRendered(
            [[maybe_unused]] const AZ::EntityId& entityId,
            [[maybe_unused]] const AZ::Render::MaterialAssignmentId& materialAssignmentId,
            [[maybe_unused]] const QPixmap& pixmap)
        {
            // Since the "preview rendered" event is not called on the main thread, queue
            // the handling code to be executed on the main thread. This prevents any non
            // thread-safe code, such as Qt updates, from running on alternate threads
            AZ::SystemTickBus::QueueFunction(
                [=]()
                {
                    PurgePreviews();

                    // Caching preview so the image will not have to be regenerated every time it's requested
                    m_materialPreviews[entityId][materialAssignmentId] = pixmap;

                    AZ::Render::EditorMaterialSystemComponentNotificationBus::Broadcast(
                        &AZ::Render::EditorMaterialSystemComponentNotificationBus::Events::OnRenderMaterialPreviewReady,
                        entityId,
                        materialAssignmentId,
                        pixmap);
                });
        }

        void EditorMaterialSystemComponent::OnMaterialSlotLayoutChanged()
        {
            // Deleting any preview saved for an entity whose material configuration is about to be invalidated
            const AZ::EntityId entityId = *MaterialComponentNotificationBus::GetCurrentBusId();
            m_materialPreviews.erase(entityId);
        }

        void EditorMaterialSystemComponent::NotifyRegisterViews()
        {
            AzToolsFramework::ViewPaneOptions inspectorOptions;
            inspectorOptions.canHaveMultipleInstances = true;
            inspectorOptions.preferedDockingArea = Qt::NoDockWidgetArea;
            inspectorOptions.paneRect = QRect(50, 50, 600, 1000);
            inspectorOptions.showInMenu = false;
            inspectorOptions.showOnToolsToolbar = false;
            AzToolsFramework::RegisterViewPane<AZ::Render::EditorMaterialComponentInspector::MaterialPropertyInspector>(
                "Material Instance Editor", LyViewPane::CategoryTools, inspectorOptions);
        }
        
        void EditorMaterialSystemComponent::AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList&)
        {
            if (newlySelectedEntities.size() == 1)
            {
                AtomImGuiTools::AtomImGuiToolsRequestBus::Broadcast(&AtomImGuiTools::AtomImGuiToolsRequests::ShowMaterialShaderDetailsForEntity, newlySelectedEntities[0], false);
            }
            else
            {
                AtomImGuiTools::AtomImGuiToolsRequestBus::Broadcast(&AtomImGuiTools::AtomImGuiToolsRequests::ShowMaterialShaderDetailsForEntity, AZ::EntityId{}, false);
            }
        }

        AzToolsFramework::AssetBrowser::SourceFileDetails EditorMaterialSystemComponent::GetSourceFileDetails(
            const char* fullSourceFileName)
        {
            const AZStd::string_view path(fullSourceFileName);
            if (path.ends_with("physxmaterial") || path.ends_with("physicsmaterial"))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/PhysXMaterial_80.svg");
            }
            if (path.ends_with(AZ::RPI::MaterialSourceData::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/Material_80.svg");
            }
            if (path.ends_with(AZ::RPI::MaterialTypeSourceData::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/MaterialType_80.svg");
            }
            if (path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphExtensionWithDot) ||
                path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphNodeExtensionWithDot) ||
                path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/MaterialGraph_80.svg");
            }
            if (path.ends_with(AZ::RPI::BufferAsset::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/BufferAsset_80.svg");
            }
            if (path.ends_with(AZ::RPI::ShaderAsset::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/Shader_80.svg");
            }
            return AzToolsFramework::AssetBrowser::SourceFileDetails();
        }

        void EditorMaterialSystemComponent::OnActionRegistrationHook()
        {
            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            AZ_Assert(actionManagerInterface, "EditorMaterialSystemComponent - could not get ActionManagerInterface");

            auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
            AZ_Assert(hotKeyManagerInterface, "EditorMaterialSystemComponent - could not get HotKeyManagerInterface");

            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Material Editor";
                actionProperties.m_iconPath = ":/Menu/material_editor.svg";

                auto outcome = actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    MaterialEditorActionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        OpenMaterialEditor("");
                    }
                );
                AZ_Assert(outcome.IsSuccess(), "Failed to RegisterAction %s", MaterialEditorActionIdentifier.data());

                hotKeyManagerInterface->SetActionHotKey(MaterialEditorActionIdentifier, "Ctrl+M");
            }

            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Material Canvas";
                actionProperties.m_iconPath = ":/Menu/material_canvas.svg";

                auto outcome = actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    MaterialCanvasActionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        OpenMaterialCanvas("");
                    });
                AZ_Assert(outcome.IsSuccess(), "Failed to RegisterAction %s", MaterialCanvasActionIdentifier.data());

                outcome = hotKeyManagerInterface->SetActionHotKey(MaterialCanvasActionIdentifier, "Ctrl+Shift+M");
                AZ_Assert(outcome.IsSuccess(), "Failed to ActionHotKey for %s", MaterialCanvasActionIdentifier.data());
            }

        }

        void EditorMaterialSystemComponent::OnMenuBindingHook()
        {
            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            AZ_Assert(actionManagerInterface, "EditorMaterialSystemComponent - could not get ActionManagerInterface");

            auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
            AZ_Assert(menuManagerInterface, "EditorMaterialSystemComponent - could not get MenuManagerInterface");

            {
                auto outcome = menuManagerInterface->AddActionToMenu(
                    EditorIdentifiers::ToolsMenuIdentifier,
                    MaterialEditorActionIdentifier,
                    actionManagerInterface->GenerateActionAlphabeticalSortKey(MaterialEditorActionIdentifier));
                AZ_Assert(
                    outcome.IsSuccess(),
                    "Failed to AddAction %s to Menu %s",
                    MaterialEditorActionIdentifier.data(),
                    EditorIdentifiers::ToolsMenuIdentifier.data());
            }

            {
                auto outcome = menuManagerInterface->AddActionToMenu(
                    EditorIdentifiers::ToolsMenuIdentifier,
                    MaterialCanvasActionIdentifier,
                    actionManagerInterface->GenerateActionAlphabeticalSortKey(MaterialCanvasActionIdentifier));
                AZ_Assert(
                    outcome.IsSuccess(),
                    "Failed to AddAction %s to Menu %s",
                    MaterialCanvasActionIdentifier.data(),
                    EditorIdentifiers::ToolsMenuIdentifier.data());
            }
        }

        void EditorMaterialSystemComponent::PurgePreviews()
        {
            // Deleting all saved previews after a certain threshold has been reached
            size_t materialPreviewCount = 0;
            for (const auto& materialPreviewPair : m_materialPreviews)
            {
                materialPreviewCount += materialPreviewPair.second.size();
            }

            if (materialPreviewCount > MaterialPreviewLimit)
            {
                m_materialPreviews.clear();
            }
        }
    } // namespace Render
} // namespace AZ
