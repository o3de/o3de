/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ReflectionProbe/EditorReflectionProbeComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/IO/SystemFile.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/Utils/DdsFile.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        void EditorReflectionProbeComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorReflectionProbeComponent, BaseClass>()
                    ->Version(2, ConvertToEditorRenderComponentAdapter<1>)
                    ->Field("useBakedCubemap", &EditorReflectionProbeComponent::m_useBakedCubemap)
                    ->Field("bakedCubeMapQualityLevel", &EditorReflectionProbeComponent::m_bakedCubeMapQualityLevel)
                    ->Field("bakedCubeMapRelativePath", &EditorReflectionProbeComponent::m_bakedCubeMapRelativePath)
                    ->Field("authoredCubeMapAsset", &EditorReflectionProbeComponent::m_authoredCubeMapAsset)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorReflectionProbeComponent>(
                        "Reflection Probe", "The ReflectionProbe component captures an IBL specular reflection at a specific position in the level")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/reflection-probe/")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::ModelAsset>::Uuid())
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Cubemap Bake")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->UIElement(AZ::Edit::UIHandlers::Button, "Bake Reflection Probe", "Bake Reflection Probe")
                                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                                ->Attribute(AZ::Edit::Attributes::ButtonText, "Bake Reflection Probe")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorReflectionProbeComponent::BakeReflectionProbe)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Cubemap")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorReflectionProbeComponent::m_useBakedCubemap, "Use Baked Cubemap", "Selects between a cubemap that captures the environment at location in the scene or a preauthored cubemap")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorReflectionProbeComponent::OnUseBakedCubemapValidate)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorReflectionProbeComponent::OnUseBakedCubemapChanged)
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorReflectionProbeComponent::m_bakedCubeMapQualityLevel, "Baked Cubemap Quality", "Resolution of the baked cubemap")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting)
                                ->EnumAttribute(BakedCubeMapQualityLevel::VeryLow, "Very Low")
                                ->EnumAttribute(BakedCubeMapQualityLevel::Low, "Low")
                                ->EnumAttribute(BakedCubeMapQualityLevel::Medium, "Medium")
                                ->EnumAttribute(BakedCubeMapQualityLevel::High, "High")
                                ->EnumAttribute(BakedCubeMapQualityLevel::VeryHigh, "Very High")
                            ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorReflectionProbeComponent::m_bakedCubeMapRelativePath, "Baked Cubemap Path", "Baked Cubemap Path")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorReflectionProbeComponent::m_authoredCubeMapAsset, "Cubemap file", "Authored Cubemap file")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetAuthoredCubemapVisibilitySetting)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorReflectionProbeComponent::OnAuthoredCubemapChanged)
                        ;

                    editContext->Class<ReflectionProbeComponentController>(
                        "ReflectionProbeComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<ReflectionProbeComponentConfig>(
                        "ReflectionProbeComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Inner Extents")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentConfig::m_innerHeight, "Height", "Height of the reflection probe inner volume")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentConfig::m_innerLength, "Length", "Length of the reflection probe inner volume")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentConfig::m_innerWidth, "Width", "Width of the reflection probe inner volume")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Settings")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ReflectionProbeComponentConfig::m_useParallaxCorrection, "Parallax Correction", "Correct the reflection to adjust for the offset from the capture position")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ReflectionProbeComponentConfig::m_showVisualization, "Show Visualization", "Show the reflection probe visualization sphere")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorReflectionProbeBus>("EditorReflectionProbeBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("BakeReflectionProbe", &EditorReflectionProbeInterface::BakeReflectionProbe)
                    ;
                
                behaviorContext->ConstantProperty("EditorReflectionProbeComponentTypeId", BehaviorConstant(Uuid(EditorReflectionProbeComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorReflectionProbeComponent::EditorReflectionProbeComponent(const ReflectionProbeComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorReflectionProbeComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
            EditorReflectionProbeBus::Handler::BusConnect(GetEntityId());
            AZ::TickBus::Handler::BusConnect();

            ReflectionProbeComponentConfig& configuration = m_controller.m_configuration;

            // update UI cubemap path display
            m_bakedCubeMapRelativePath = configuration.m_bakedCubeMapRelativePath;

            AZ::u64 entityId = (AZ::u64)GetEntityId();
            configuration.m_entityId = entityId;
        }

        void EditorReflectionProbeComponent::Deactivate()
        {
            EditorReflectionProbeBus::Handler::BusDisconnect(GetEntityId());
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorReflectionProbeComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!m_controller.m_featureProcessor)
            {
                return;
            }

            if (m_controller.m_configuration.m_useBakedCubemap)
            {
                AZStd::string cubeMapRelativePath = m_controller.m_configuration.m_bakedCubeMapRelativePath + ".streamingimage";
                Data::Asset<RPI::StreamingImageAsset> cubeMapAsset;
                CubeMapAssetNotificationType notificationType = CubeMapAssetNotificationType::None;
                if (m_controller.m_featureProcessor->CheckCubeMapAssetNotification(cubeMapRelativePath, cubeMapAsset, notificationType))
                {
                    // a cubemap bake is in progress for this entity component
                    if (notificationType == CubeMapAssetNotificationType::Ready)
                    {
                        // bake is complete, update configuration with the new baked cubemap asset
                        m_controller.m_configuration.m_bakedCubeMapAsset = { cubeMapAsset.GetAs<RPI::StreamingImageAsset>(), AZ::Data::AssetLoadBehavior::PreLoad };

                        // refresh the currently rendered cubemap
                        m_controller.UpdateCubeMap();

                        // update the UI
                        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
                    }
                    else if (notificationType == CubeMapAssetNotificationType::Error)
                    {
                        // cubemap bake failed
                        QMessageBox::information(
                            QApplication::activeWindow(),
                            "Reflection Probe",
                            "Reflection Probe cubemap failed to bake, please check the Asset Processor for more information.",
                            QMessageBox::Ok);

                        // clear relative path, this will allow the user to retry
                        m_controller.m_configuration.m_bakedCubeMapRelativePath.clear();

                        // update the UI
                        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
                    }
                }
            }
        }

        void EditorReflectionProbeComponent::DisplayEntityViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
        {
            // only draw the bounds if selected
            if (!IsSelected())
            {
                return;
            }

            AZ::Vector3 position = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
            AZ::Quaternion rotationQuaternion = AZ::Quaternion::CreateIdentity();
            AZ::TransformBus::EventResult(rotationQuaternion, GetEntityId(), &AZ::TransformBus::Events::GetWorldRotationQuaternion);
            AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(rotationQuaternion);

            float scale = 1.0f;
            AZ::TransformBus::EventResult(scale, GetEntityId(), &AZ::TransformBus::Events::GetLocalUniformScale);

            // draw AABB at probe position using the inner dimensions
            Color color(0.0f, 0.0f, 1.0f, 1.0f);
            debugDisplay.SetColor(color);

            ReflectionProbeComponentConfig& configuration = m_controller.m_configuration;
            AZ::Vector3 innerExtents(configuration.m_innerWidth, configuration.m_innerLength, configuration.m_innerHeight);
            innerExtents *= scale;

            debugDisplay.DrawWireOBB(position, rotationMatrix.GetBasisX(), rotationMatrix.GetBasisY(), rotationMatrix.GetBasisZ(), innerExtents / 2.0f);
        }

        AZ::Aabb EditorReflectionProbeComponent::GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return m_controller.GetWorldBounds();
        }

        bool EditorReflectionProbeComponent::SupportsEditorRayIntersect()
        {
            return false;
        }

        AZ::Outcome<void, AZStd::string> EditorReflectionProbeComponent::OnUseBakedCubemapValidate([[maybe_unused]] void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("This Reflection Probe entity is hidden, it must be visible in order to change the cubemap type."));
            }

            return AZ::Success();
        }

        AZ::u32 EditorReflectionProbeComponent::OnUseBakedCubemapChanged()
        {
            // save setting to the configuration
            m_controller.m_configuration.m_useBakedCubemap = m_useBakedCubemap;

            // refresh currently displayed cubemap
            m_controller.UpdateCubeMap();

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 EditorReflectionProbeComponent::OnAuthoredCubemapChanged()
        {
            // save the selected authored asset to the configuration
            m_controller.m_configuration.m_authoredCubeMapAsset = m_authoredCubeMapAsset;

            // refresh currently displayed cubemap
            m_controller.UpdateCubeMap();

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting()
        {
            // controls specific to baked cubemaps call this to determine their visibility
            // they are visible when the mode is set to baked, otherwise hidden
            return m_useBakedCubemap ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::u32 EditorReflectionProbeComponent::GetAuthoredCubemapVisibilitySetting()
        {
            // controls specific to authored cubemaps call this to determine their visibility
            // they are hidden when the mode is set to baked, otherwise visible
            return m_useBakedCubemap ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
        }

        AZ::u32 EditorReflectionProbeComponent::BakeReflectionProbe()
        {
            if (!m_useBakedCubemap)
            {
                AZ_Assert(false, "BakeReflectionProbe() called on a Reflection Probe set to use an authored cubemap");
                return AZ::Edit::PropertyRefreshLevels::None;
            }

            if (m_bakeInProgress)
            {
                return AZ::Edit::PropertyRefreshLevels::None;
            }

            // retrieve entity visibility
            bool isHidden = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                isHidden,
                GetEntityId(),
                &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsHidden);

            // the entity must be visible in order to bake
            if (isHidden)
            {
                QMessageBox::information(
                    QApplication::activeWindow(),
                    "Reflection Probe",
                    "This Reflection Probe entity is hidden, it must be visible in order to bake the cubemap.",
                    QMessageBox::Ok);

                return AZ::Edit::PropertyRefreshLevels::None;
            }

            char projectPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectPath, AZ_MAX_PATH_LEN);

            // retrieve the source cubemap path from the configuration
            // we need to make sure to use the same source cubemap for each bake
            AZStd::string cubeMapRelativePath = m_controller.m_configuration.m_bakedCubeMapRelativePath;
            AZStd::string cubeMapFullPath;

            if (!cubeMapRelativePath.empty())
            {
                // test to see if the cubemap file is actually there, if it was removed we need to
                // generate a new filename, otherwise it will cause an error in the asset system
                AzFramework::StringFunc::Path::Join(projectPath, cubeMapRelativePath.c_str(), cubeMapFullPath, true, true);

                if (!AZ::IO::FileIOBase::GetInstance()->Exists(cubeMapFullPath.c_str()))
                {
                    // clear it to force the generation of a new filename
                    cubeMapRelativePath.clear();
                }

                // if the quality level changed we need to generate a new filename
                if (m_controller.m_configuration.m_bakedCubeMapQualityLevel != m_bakedCubeMapQualityLevel)
                {
                    cubeMapRelativePath.clear();
                }
            }

            // build a new cubemap path if necessary
            if (cubeMapRelativePath.empty())
            {
                // the file name is a combination of the entity name, a UUID, and the filemask
                Entity* entity = GetEntity();
                AZ_Assert(entity, "ReflectionProbe entity is null");

                AZ::Uuid uuid = AZ::Uuid::CreateRandom();
                AZStd::string uuidString;
                uuid.ToString(uuidString);

                // determine the filemask suffix from the cubemap quality level setting
                AZStd::string fileSuffix = BakedCubeMapFileSuffixes[aznumeric_cast<uint32_t>(m_bakedCubeMapQualityLevel)];

                cubeMapRelativePath = "ReflectionProbes/" + entity->GetName() + "_" + uuidString + fileSuffix;

                // replace any invalid filename characters
                auto invalidCharacters = [](char letter)
                {
                    return
                        letter == ':' || letter == '"' || letter == '\'' ||
                        letter == '{' || letter == '}' ||
                        letter == '<' || letter == '>';
                };
                AZStd::replace_if(cubeMapRelativePath.begin(), cubeMapRelativePath.end(), invalidCharacters, '_');

                // build the full source path
                AzFramework::StringFunc::Path::Join(projectPath, cubeMapRelativePath.c_str(), cubeMapFullPath, true, true);
            }

            // make sure the folder is created
            AZStd::string reflectionProbeFolder;
            AzFramework::StringFunc::Path::GetFolderPath(cubeMapFullPath.data(), reflectionProbeFolder);
            AZ::IO::SystemFile::CreateDir(reflectionProbeFolder.c_str());

            // check out the file in source control                
            bool checkedOutSuccessfully = false;
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            ApplicationBus::BroadcastResult(
                checkedOutSuccessfully,
                &ApplicationBus::Events::RequestEditForFileBlocking,
                cubeMapFullPath.c_str(),
                "Checking out for edit...",
                ApplicationBus::Events::RequestEditProgressCallback());

            if (!checkedOutSuccessfully)
            {
                AZ_Error("ReflectionProbe", false, "Failed to write \"%s\", source control checkout failed", cubeMapFullPath.c_str());
            }

            // save the relative source path in the configuration
            AzToolsFramework::ScopedUndoBatch undoBatch("Cubemap path changed.");
            m_controller.m_configuration.m_bakedCubeMapRelativePath = cubeMapRelativePath;
            m_controller.m_configuration.m_bakedCubeMapQualityLevel = m_bakedCubeMapQualityLevel;
            SetDirty();

            // update UI cubemap path display
            m_bakedCubeMapRelativePath = cubeMapRelativePath;

            // callback from the EnvironmentCubeMapPass when the cubemap render is complete
            BuildCubeMapCallback buildCubeMapCallback = [=](uint8_t* const* cubeMapFaceTextureData, const RHI::Format cubeMapTextureFormat)
            {
                // write the cubemap data to the .dds file
                WriteOutputFile(cubeMapFullPath.c_str(), cubeMapFaceTextureData, cubeMapTextureFormat);
                m_bakeInProgress = false;
            };

            // initiate the cubemap bake, this will invoke the buildCubeMapCallback when the cubemap data is ready
            m_bakeInProgress = true;
            AZStd::string cubeMapRelativeAssetPath = cubeMapRelativePath + ".streamingimage";
            m_controller.BakeReflectionProbe(buildCubeMapCallback, cubeMapRelativeAssetPath);

            // show a dialog box letting the user know the probe is baking
            QProgressDialog bakeDialog;
            bakeDialog.setWindowFlags(bakeDialog.windowFlags() & ~Qt::WindowCloseButtonHint);
            bakeDialog.setLabelText("Baking Reflection Probe...");
            bakeDialog.setWindowModality(Qt::WindowModal);
            bakeDialog.setMaximumSize(QSize(256, 96));
            bakeDialog.setMinimum(0);
            bakeDialog.setMaximum(0);
            bakeDialog.setMinimumDuration(0);
            bakeDialog.setAutoClose(false);
            bakeDialog.setCancelButton(nullptr);
            bakeDialog.show();

            // display until finished or canceled
            while (m_bakeInProgress)
            {
                if (bakeDialog.wasCanceled())
                {
                    m_bakeInProgress = false;
                    break;
                }

                QApplication::processEvents();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            }

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        void EditorReflectionProbeComponent::WriteOutputFile(AZStd::string filePath, uint8_t* const* cubeMapTextureData, const RHI::Format cubeMapTextureFormat)
        {
            static const u32 CubeMapFaceSize = 1024;
            static const u32 NumCubeMapFaces = 6;

            u32 bytesPerTexel = RHI::GetFormatSize(cubeMapTextureFormat);
            u32 bytesPerCubeMapFace = CubeMapFaceSize * CubeMapFaceSize * bytesPerTexel;

            AZStd::vector<uint8_t> buffer;
            buffer.resize_no_construct(bytesPerCubeMapFace * NumCubeMapFaces);
            for (AZ::u32 i = 0; i < NumCubeMapFaces; ++i)
            {
                memcpy(buffer.data() + (i * bytesPerCubeMapFace), cubeMapTextureData[i], bytesPerCubeMapFace);
            }

            DdsFile::DdsFileData ddsFileData;
            ddsFileData.m_size.m_width = CubeMapFaceSize;
            ddsFileData.m_size.m_height = CubeMapFaceSize;
            ddsFileData.m_format = cubeMapTextureFormat;
            ddsFileData.m_isCubemap = true;
            ddsFileData.m_buffer = &buffer;

            auto outcome = AZ::DdsFile::WriteFile(filePath, ddsFileData);
            if (!outcome)
            {
                AZ_Warning("WriteDds", false, outcome.GetError().m_message.c_str());
            }
        }
    } // namespace Render
} // namespace AZ
