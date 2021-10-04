/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Assets/HairAsset.h>
#include <Builders/HairAssetBuilder.h>
#include <AssetBuilderSDK/SerializationDependencies.h>

#include <AzCore/IO/FileIO.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            void HairAssetBuilder::RegisterBuilder()
            {
                // Setup the builder descriptor
                AssetBuilderSDK::AssetBuilderDesc builderDesc;
                builderDesc.m_name = "HairAssetBuilder";

                builderDesc.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern(
                    AZStd::string::format("*.%s", AMD::TFXFileExtension), AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
                builderDesc.m_busId = azrtti_typeid<HairAssetBuilder>();
                builderDesc.m_version = 3;
                builderDesc.m_createJobFunction =
                    AZStd::bind(&HairAssetBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
                builderDesc.m_processJobFunction =
                    AZStd::bind(&HairAssetBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

                BusConnect(builderDesc.m_busId);

                AssetBuilderSDK::AssetBuilderBus::Broadcast(
                    &AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDesc);
            }

            void HairAssetBuilder::ShutDown()
            {
                m_isShuttingDown = true;
            }

            void HairAssetBuilder::CreateJobs(
                const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
            {
                if (m_isShuttingDown)
                {
                    response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                    return;
                }

                for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
                {
                    AssetBuilderSDK::JobDescriptor descriptor;
                    descriptor.m_jobKey = AMD::TFXFileExtension;
                    descriptor.m_critical = false;
                    descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
                    response.m_createJobOutputs.push_back(descriptor);
                }

                // Set the tfx bone and tfx mesh file as source dependency. This way when tfxbone or tfxmesh file reloaded it will
                // also trigger the rebuild of the hair asset.
                AssetBuilderSDK::SourceFileDependency sourceFileDependency;
                sourceFileDependency.m_sourceFileDependencyPath = request.m_sourceFile;
                AZ::StringFunc::Path::ReplaceExtension(sourceFileDependency.m_sourceFileDependencyPath, AMD::TFXBoneFileExtension);
                response.m_sourceFileDependencyList.push_back(sourceFileDependency);

                sourceFileDependency.m_sourceFileDependencyPath = request.m_sourceFile;
                AZ::StringFunc::Path::ReplaceExtension(sourceFileDependency.m_sourceFileDependencyPath, AMD::TFXMeshFileExtension);
                response.m_sourceFileDependencyList.push_back(sourceFileDependency);

                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            }

            void HairAssetBuilder::ProcessJob(
                const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
            {
                AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "HairAssetBuilder Starting Job for %s.\n", request.m_fullPath.c_str());

                if (m_isShuttingDown)
                {
                    AZ_TracePrintf(
                        AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    return;
                }

                // There are 3 source files for TressFX asset - .tfx, .tfxbone and .tfxmesh.
                // We read all the 3 source files and combine them to 1 output file .tfxhair in the cache.
                AZStd::string tfxFileName;
                AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), tfxFileName);

                // Create the path to the result .tfxhair file inside the tempDirPath.
                AZStd::string destPath;
                AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), tfxFileName.c_str(), destPath, true);
                AZ::StringFunc::Path::ReplaceExtension(destPath, AMD::TFXCombinedFileExtension);

                // Create and open the .tfxhair we are writing to. 
                AZ::IO::FileIOStream outStream(destPath.data(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
                if (!outStream.IsOpen())
                {
                    AZ_TracePrintf(
                        AssetBuilderSDK::ErrorWindow, "Error: Failed job %s because .tfxhair file cannot be created.\n", request.m_fullPath.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                // Write the .tfxhair file header as a placeholder
                AMD::TressFXCombinedHairFileHeader header; 
                outStream.Write(sizeof(AMD::TressFXCombinedHairFileHeader), &header);

                // Combine .tfx, .tfxbone, .tfxmesh file into .tfxhair file
                auto writeToStream = [](const AZStd::string& fullpath, AZ::IO::FileIOStream& outStream, bool required) {
                    IO::FileIOStream inStream;
                    if (!inStream.Open(fullpath.c_str(), IO::OpenMode::ModeRead))
                    {
                        if (required)
                        {
                            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow,
                                "Error: Failed job %s because the file is either missing or cannot be opened.\n", fullpath.c_str());
                        }
                        return (AZ::IO::SizeType)0;
                    }
                    const AZ::IO::SizeType dataSize = inStream.GetLength();
                    AZStd::vector<AZ::u8> fileBuffer(dataSize);
                    inStream.Read(dataSize, fileBuffer.data());
                    return outStream.Write(dataSize, fileBuffer.data());
                };

                // Write .tfx file to the combined .tfxhair file.
                AZStd::string sourcePath = request.m_fullPath;
                const AZ::IO::SizeType tfxSize = writeToStream(sourcePath, outStream, true);

                // Move on to .tfxbone file.
                AZ::StringFunc::Path::ReplaceExtension(sourcePath, AMD::TFXBoneFileExtension);
                const AZ::IO::SizeType tfxBoneSize = writeToStream(sourcePath, outStream, true);

                // Move on to .tfxmesh file.
                AZ::StringFunc::Path::ReplaceExtension(sourcePath, AMD::TFXMeshFileExtension);
                writeToStream(sourcePath, outStream, false);

                if (tfxSize == 0 || tfxBoneSize == 0)
                {
                    // Fail the job if the .tfx file or the .tfxbone file is missing.
                    AZ_TracePrintf(AssetBuilderSDK::ErrorWindow,
                        "Error: Failed job %s because tfxSize=%d or tfxBoneSize=%d.\n",
                        request.m_fullPath.c_str(), tfxSize, tfxBoneSize);
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                // Write the header file with correct data
                header.offsetTFX = sizeof(AMD::TressFXCombinedHairFileHeader);
                header.offsetTFXBone = header.offsetTFX + tfxSize;
                header.offsetTFXMesh = header.offsetTFXBone + tfxBoneSize;
                outStream.Seek(0, AZ::IO::FileIOStream::SeekMode::ST_SEEK_BEGIN);
                outStream.Write(sizeof(AMD::TressFXCombinedHairFileHeader), &header);
                
                // Send the .tfxhair as the final job product product.
                AssetBuilderSDK::JobProduct jobProduct(destPath, azrtti_typeid<HairAsset>(), 0);
                jobProduct.m_dependenciesHandled = true; 
                response.m_outputProducts.push_back(jobProduct);
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

                AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "HairAssetBuilder successfully finished Job for %s.\n", request.m_fullPath.c_str());
            }
        }
    } // namespace Render
} // namespace AZ
