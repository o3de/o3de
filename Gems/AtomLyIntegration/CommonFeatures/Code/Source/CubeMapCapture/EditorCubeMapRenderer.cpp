/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <CubeMapCapture/EditorCubeMapRenderer.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        AZ::u32 EditorCubeMapRenderer::RenderCubeMap(
            AZStd::function<void(RenderCubeMapCallback, AZStd::string&)> renderCubeMapFn,
            const AZStd::string dialogText,
            const AZ::Entity* entity,
            const AZStd::string& folderName,
            AZStd::string& relativePath,
            CubeMapCaptureType captureType,
            CubeMapSpecularQualityLevel specularQualityLevel /* = CubeMapSpecularQualityLevel::Medium */)
        {
            if (m_renderInProgress)
            {
                return AZ::Edit::PropertyRefreshLevels::None;
            }

            // retrieve entity visibility
            bool isHidden = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                isHidden,
                entity->GetId(),
                &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsHidden);

            // the entity must be visible in order to capture
            if (isHidden)
            {
                QMessageBox::information(
                    QApplication::activeWindow(),
                    "Error",
                    "Entity must be visible to build the cubemap.",
                    QMessageBox::Ok);

                return AZ::Edit::PropertyRefreshLevels::None;
            }

            char projectPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectPath, AZ_MAX_PATH_LEN);

            // retrieve the source cubemap path from the configuration
            // we need to make sure to use the same source cubemap for each capture
            AZStd::string cubeMapRelativePath = relativePath;
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
            }

            // build a new cubemap path if necessary
            if (cubeMapRelativePath.empty())
            {
                // the file name is a combination of the entity name, a UUID, and the filemask
                AZ::Uuid uuid = AZ::Uuid::CreateRandom();
                AZStd::string uuidString;
                uuid.ToString(uuidString);

                // determine the file suffix
                AZStd::string fileSuffix;
                if (captureType == CubeMapCaptureType::Specular)
                {
                    fileSuffix = CubeMapSpecularFileSuffixes[aznumeric_cast<uint32_t>(specularQualityLevel)];
                }
                else
                {
                    fileSuffix = CubeMapDiffuseFileSuffix;
                }

                cubeMapRelativePath = folderName + "/" + entity->GetName() + "_" + uuidString + fileSuffix;

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
            AZStd::string cubemapCaptureFolderPath;
            AzFramework::StringFunc::Path::GetFolderPath(cubeMapFullPath.data(), cubemapCaptureFolderPath);
            AZ::IO::SystemFile::CreateDir(cubemapCaptureFolderPath.c_str());

            // check out the file in source control                
            bool checkedOutSuccessfully = false;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                checkedOutSuccessfully,
                &AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditForFileBlocking,
                cubeMapFullPath.c_str(),
                "Checking out for edit...",
                AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditProgressCallback());

            if (!checkedOutSuccessfully)
            {
                AZ_Error("CubeMapCapture", false, "Source control checkout failed for file [%s]", cubeMapFullPath.c_str());
            }

            // save the relative source path in the configuration
            relativePath = cubeMapRelativePath;

            // callback from the EnvironmentCubeMapPass when the cubemap render is complete
            RenderCubeMapCallback renderCubeMapCallback = [=](uint8_t* const* cubeMapFaceTextureData, const RHI::Format cubeMapTextureFormat)
            {
                // write the cubemap data to the .dds file
                WriteOutputFile(cubeMapFullPath.c_str(), cubeMapFaceTextureData, cubeMapTextureFormat);
                m_renderInProgress = false;
            };

            // initiate the cubemap bake, this will invoke the buildCubeMapCallback when the cubemap data is ready
            m_renderInProgress = true;
            AZStd::string cubeMapRelativeAssetPath = cubeMapRelativePath + ".streamingimage";
            renderCubeMapFn(renderCubeMapCallback, cubeMapRelativeAssetPath);

            // show a dialog box letting the user know the cubemap is capturing
            QProgressDialog captureDialog;
            captureDialog.setWindowFlags(captureDialog.windowFlags() & ~Qt::WindowCloseButtonHint);
            captureDialog.setLabelText(dialogText.c_str());
            captureDialog.setWindowModality(Qt::WindowModal);
            captureDialog.setMaximumSize(QSize(256, 96));
            captureDialog.setMinimum(0);
            captureDialog.setMaximum(0);
            captureDialog.setMinimumDuration(0);
            captureDialog.setAutoClose(false);
            captureDialog.setCancelButton(nullptr);
            captureDialog.show();

            // display until finished or canceled
            while (m_renderInProgress)
            {
                if (captureDialog.wasCanceled())
                {
                    m_renderInProgress = false;
                    break;
                }

                QApplication::processEvents();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            }

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        void EditorCubeMapRenderer::WriteOutputFile(AZStd::string filePath, uint8_t* const* cubeMapTextureData, const RHI::Format cubeMapTextureFormat)
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
    }
}
