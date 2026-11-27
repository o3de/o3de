/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CanvasUpgrader.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <UiCanvasFileObject.h>
#include <UiPrefabInstance.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
AZ_POP_DISABLE_WARNING

namespace LYShineToShine
{
    CanvasUpgrader::UpgradeReport CanvasUpgrader::UpgradeDirectory(const AZ::IO::Path& directoryPath)
    {
        UpgradeReport report;

        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            report.m_failureDetails.emplace_back("(system)", "FileIO not available");
            return report;
        }

        // Recursively find all .uicanvas files
        AZStd::vector<AZ::IO::Path> canvasFiles;
        fileIO->FindFiles(directoryPath.c_str(), "*.uicanvas",
            [&canvasFiles](const char* filePath) -> bool
            {
                canvasFiles.emplace_back(filePath);
                return true; // continue searching
            });

        // Also search subdirectories
        AZStd::function<void(const AZ::IO::Path&)> searchSubdirs;
        searchSubdirs = [&](const AZ::IO::Path& dir)
        {
            fileIO->FindFiles(dir.c_str(), "*",
                [&](const char* entry) -> bool
                {
                    AZ::IO::Path entryPath(entry);
                    if (fileIO->IsDirectory(entryPath.c_str()))
                    {
                        // Search this subdirectory for .uicanvas files
                        fileIO->FindFiles(entryPath.c_str(), "*.uicanvas",
                            [&canvasFiles](const char* filePath) -> bool
                            {
                                canvasFiles.emplace_back(filePath);
                                return true;
                            });
                        // Recurse into subdirectory
                        searchSubdirs(entryPath);
                    }
                    return true;
                });
        };
        searchSubdirs(directoryPath);

        AZ_TracePrintf("LYShineToShine", "Found %zu .uicanvas files\n", canvasFiles.size());

        for (const auto& filePath : canvasFiles)
        {
            report.m_filesScanned++;

            AZStd::string error;
            if (!UpgradeFile(filePath, error))
            {
                if (error == "already_upgraded")
                {
                    report.m_alreadyUpgraded++;
                }
                else
                {
                    report.m_failed++;
                    report.m_failureDetails.emplace_back(filePath.String(), error);
                }
            }
            else
            {
                if (error == "upgraded_with_slicerefs")
                {
                    report.m_upgradedWithSliceRefs++;
                }
                else
                {
                    report.m_upgradedSimple++;
                }
            }
        }

        return report;
    }

    bool CanvasUpgrader::UpgradeFile(const AZ::IO::Path& filePath, AZStd::string& outError)
    {
        // Resolve alias path to absolute path
        AZ::IO::Path resolvedPath = filePath;
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            char resolvedBuffer[AZ::IO::MaxPathLength];
            if (fileIO->ResolvePath(filePath.c_str(), resolvedBuffer, AZ::IO::MaxPathLength))
            {
                resolvedPath = resolvedBuffer;
            }
        }

        // Read the entire file
        AZ::IO::SystemFile file;
        if (!file.Open(resolvedPath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            outError = AZStd::string::format("Failed to open file for reading: %s", resolvedPath.c_str());
            return false;
        }

        AZ::IO::SizeType fileSize = file.Length();
        AZStd::string xmlContent;
        xmlContent.resize_no_construct(fileSize);
        file.Read(fileSize, xmlContent.data());
        file.Close();

        // Detect version
        int version = DetectVersion(xmlContent);

        if (version == 3 || version == 4)
        {
            outError = "already_upgraded";
            return false;
        }

        if (version == 0)
        {
            outError = "Unrecognized canvas format";
            return false;
        }

        // Check if this canvas has SliceReferences (complex case)
        if (HasSliceReferences(xmlContent))
        {
            bool result = UpgradeWithSliceRefs(xmlContent, resolvedPath, outError);
            if (result)
            {
                outError = "upgraded_with_slicerefs";
            }
            return result;
        }
        else
        {
            return UpgradeSimple(xmlContent, resolvedPath, outError);
        }
    }

    int CanvasUpgrader::DetectVersion(const AZStd::string& xmlContent) const
    {
        // Look for the canvas file object version attribute. Very old files use the
        // class name "CanvasFileObject" instead of "UiCanvasFileObject" (same type UUID),
        // and "CanvasFileObject" is a substring of both, so search for that.
        auto pos = xmlContent.find("CanvasFileObject");
        if (pos == AZStd::string::npos)
        {
            return 0;
        }

        // Find version="N" after the canvas file object class name
        auto versionPos = xmlContent.find("version=\"", pos);
        if (versionPos == AZStd::string::npos || versionPos > pos + 100)
        {
            return 0;
        }

        versionPos += 9; // skip past version="
        if (versionPos < xmlContent.size())
        {
            char versionChar = xmlContent[versionPos];
            if (versionChar >= '1' && versionChar <= '4')
            {
                return versionChar - '0';
            }
        }

        return 0;
    }

    bool CanvasUpgrader::HasSliceReferences(const AZStd::string& xmlContent) const
    {
        // Old files use "PrefabReference", newer files use "SliceReference" -- same type UUID
        return xmlContent.find("SliceReference") != AZStd::string::npos
            || xmlContent.find("PrefabReference") != AZStd::string::npos;
    }

    AZStd::string CanvasUpgrader::ExtractXmlBlock(const AZStd::string& xml, const char* fieldName, size_t startSearchPos) const
    {
        // Find field="fieldName" in a <Class ...> tag
        AZStd::string fieldPattern = AZStd::string::format("field=\"%s\"", fieldName);
        auto fieldPos = xml.find(fieldPattern, startSearchPos);
        if (fieldPos == AZStd::string::npos)
        {
            return {};
        }

        // Walk back to find the opening <Class
        auto openTagStart = xml.rfind("<Class ", fieldPos);
        if (openTagStart == AZStd::string::npos)
        {
            return {};
        }

        // Now we need to find the matching close tag, handling nested <Class>...</Class> pairs
        int depth = 0;
        size_t pos = openTagStart;
        size_t blockEnd = AZStd::string::npos;

        while (pos < xml.size())
        {
            auto nextOpen = xml.find("<Class ", pos + 1);
            [[maybe_unused]] auto nextSelfClose = xml.find("/>", pos);
            auto nextClose = xml.find("</Class>", pos);

            // Check if current tag is self-closing (before any nested open)
            if (depth == 0)
            {
                // Find the end of the opening tag
                auto tagEnd = xml.find('>', openTagStart);
                if (tagEnd != AZStd::string::npos && tagEnd >= 2 && xml[tagEnd - 1] == '/')
                {
                    // Self-closing tag
                    blockEnd = tagEnd + 1;
                    break;
                }
                depth = 1;
                pos = tagEnd != AZStd::string::npos ? tagEnd + 1 : pos + 1;
                continue;
            }

            // Find the earliest event
            size_t earliest = AZStd::string::npos;
            enum Event { None, Open, SelfClose, Close } event = None;

            if (nextOpen != AZStd::string::npos && nextOpen < earliest) { earliest = nextOpen; event = Open; }
            if (nextClose != AZStd::string::npos && nextClose < earliest) { earliest = nextClose; event = Close; }

            if (event == None)
            {
                break; // malformed
            }

            if (event == Open)
            {
                // Check if this open tag is self-closing
                auto openEnd = xml.find('>', earliest);
                if (openEnd != AZStd::string::npos && openEnd >= 2 && xml[openEnd - 1] == '/')
                {
                    // Self-closing, doesn't change depth
                    pos = openEnd + 1;
                }
                else
                {
                    depth++;
                    pos = openEnd != AZStd::string::npos ? openEnd + 1 : earliest + 1;
                }
            }
            else if (event == Close)
            {
                depth--;
                if (depth == 0)
                {
                    blockEnd = earliest + 8; // strlen("</Class>")
                    break;
                }
                pos = earliest + 8;
            }
        }

        if (blockEnd == AZStd::string::npos)
        {
            return {};
        }

        return xml.substr(openTagStart, blockEnd - openTagStart);
    }

    bool CanvasUpgrader::UpgradeSimple(const AZStd::string& xmlContent, const AZ::IO::Path& filePath, AZStd::string& outError)
    {
        AZ_TracePrintf("LYShineToShine", "Upgrading (simple): %s\n", filePath.c_str());

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            outError = "SerializeContext not available";
            return false;
        }

        // Detect ObjectStream version
        AZStd::string streamVersion = "3";
        auto vPos = xmlContent.find("<ObjectStream version=\"");
        if (vPos != AZStd::string::npos)
        {
            streamVersion = xmlContent.substr(vPos + 23, 1);
        }

        // --- Step 1: Deserialize CanvasEntity ---
        AZStd::string canvasEntityXml = ExtractXmlBlock(xmlContent, "CanvasEntity");
        if (canvasEntityXml.empty())
        {
            outError = "Could not find CanvasEntity in file";
            return false;
        }

        AZStd::string wrappedCanvas = AZStd::string::format(
            "<ObjectStream version=\"%s\">\n%s\n</ObjectStream>\n",
            streamVersion.c_str(), canvasEntityXml.c_str());

        AZ::IO::MemoryStream canvasStream(wrappedCanvas.data(), wrappedCanvas.size());
        AZ::Entity* canvasEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(
            canvasStream, serializeContext);
        if (!canvasEntity)
        {
            outError = "Failed to deserialize CanvasEntity";
            return false;
        }

        // --- Step 2: Deserialize RootSliceEntity ---
        AZStd::string rootSliceEntityXml = ExtractXmlBlock(xmlContent, "RootSliceEntity");
        if (rootSliceEntityXml.empty())
        {
            outError = "Could not find RootSliceEntity in file";
            delete canvasEntity;
            return false;
        }

        AZStd::string wrappedSlice = AZStd::string::format(
            "<ObjectStream version=\"%s\">\n%s\n</ObjectStream>\n",
            streamVersion.c_str(), rootSliceEntityXml.c_str());

        AZ::IO::MemoryStream sliceStream(wrappedSlice.data(), wrappedSlice.size());
        AZ::Entity* rootSliceEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(
            sliceStream, serializeContext,
            AZ::ObjectStream::FilterDescriptor(&AZ::ObjectStream::AssetFilterSlicesOnly));
        if (!rootSliceEntity)
        {
            outError = "Failed to deserialize RootSliceEntity";
            delete canvasEntity;
            return false;
        }

        auto* sliceComponent = rootSliceEntity->FindComponent<AZ::SliceComponent>();
        if (!sliceComponent)
        {
            outError = "No SliceComponent found on RootSliceEntity";
            delete rootSliceEntity;
            delete canvasEntity;
            return false;
        }

        sliceComponent->SetSerializeContext(serializeContext);

        // Get entities from the SliceComponent
        const AZ::SliceComponent::EntityList& entities = sliceComponent->GetNewEntities();

        // --- Step 3: Build UiCanvasFileObject and serialize ---
        UiCanvasFileObject fileObject;
        fileObject.m_canvasEntity = canvasEntity;
        for (AZ::Entity* entity : entities)
        {
            fileObject.m_childEntities.push_back(entity);
        }

        // Serialize to a byte buffer, then write to file
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> byteStream(&byteBuffer);
        UiCanvasFileObject::SaveCanvasToStream(byteStream, &fileObject);

        AZ::IO::SystemFile outFile;
        if (!outFile.Open(filePath.c_str(),
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            outError = AZStd::string::format("Failed to open file for writing: %s", filePath.c_str());
            fileObject.m_canvasEntity = nullptr;
            fileObject.m_childEntities.clear();
            delete rootSliceEntity;
            delete canvasEntity;
            return false;
        }

        outFile.Write(byteBuffer.data(), byteBuffer.size());
        outFile.Close();

        AZ_TracePrintf("LYShineToShine", "  Upgraded: %s (%zu entities)\n", filePath.c_str(), entities.size());

        // Don't let fileObject destructor delete entities we don't own
        fileObject.m_canvasEntity = nullptr;
        fileObject.m_childEntities.clear();

        delete rootSliceEntity;
        // The canvas entity is owned by this function, not by the root slice entity.
        delete canvasEntity;
        return true;
    }

    bool CanvasUpgrader::UpgradeWithSliceRefs(
        const AZStd::string& xmlContent,
        const AZ::IO::Path& filePath,
        AZStd::string& outError)
    {
        // Phase 2: Complex canvas with SliceReferences/PrefabReferences
        //
        // Strategy:
        //   1. Get SerializeContext from the application
        //   2. Extract the CanvasEntity XML, deserialize and re-serialize to normalize to v4 format
        //   3. Extract the RootSliceEntity XML, wrap in ObjectStream, deserialize as AZ::Entity
        //   4. Find SliceComponent on the entity
        //   5. Call SliceComponent::GetEntities() (triggers Instantiate, resolves all refs + DataPatches)
        //   6. Serialize each entity to XML
        //   7. Build the v4 canvas and write to disk
        //
        // Requirements:
        //   - Must run inside the Editor (or any app with AssetManager running)
        //   - Referenced .slice files must be processed and in the asset catalog

        AZ_TracePrintf("LYShineToShine", "Upgrading (with slice refs): %s\n", filePath.c_str());

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            outError = "SerializeContext not available";
            return false;
        }

        // Detect ObjectStream version from original file
        AZStd::string streamVersion = "3";
        auto vPos = xmlContent.find("<ObjectStream version=\"");
        if (vPos != AZStd::string::npos)
        {
            streamVersion = xmlContent.substr(vPos + 23, 1);
        }

        // --- Step 1: Deserialize CanvasEntity ---
        AZStd::string canvasEntityXml = ExtractXmlBlock(xmlContent, "CanvasEntity");
        if (canvasEntityXml.empty())
        {
            outError = "Could not find CanvasEntity";
            return false;
        }

        AZStd::string wrappedCanvas = AZStd::string::format(
            "<ObjectStream version=\"%s\">\n%s\n</ObjectStream>\n",
            streamVersion.c_str(), canvasEntityXml.c_str());

        AZ::IO::MemoryStream canvasStream(wrappedCanvas.data(), wrappedCanvas.size());
        AZ::Entity* canvasEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(
            canvasStream, serializeContext);
        if (!canvasEntity)
        {
            outError = "Failed to deserialize CanvasEntity";
            return false;
        }

        // --- Step 2: Deserialize RootSliceEntity ---
        AZStd::string rootSliceEntityXml = ExtractXmlBlock(xmlContent, "RootSliceEntity");
        if (rootSliceEntityXml.empty())
        {
            outError = "Could not find RootSliceEntity";
            delete canvasEntity;
            return false;
        }

        AZStd::string wrappedSlice = AZStd::string::format(
            "<ObjectStream version=\"%s\">\n%s\n</ObjectStream>\n",
            streamVersion.c_str(), rootSliceEntityXml.c_str());

        // Use AssetFilterSlicesOnly to trigger loading of referenced slice assets during deserialization
        AZ::IO::MemoryStream sliceStream(wrappedSlice.data(), wrappedSlice.size());
        AZ::Entity* rootSliceEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(
            sliceStream, serializeContext,
            AZ::ObjectStream::FilterDescriptor(&AZ::ObjectStream::AssetFilterSlicesOnly));
        if (!rootSliceEntity)
        {
            outError = "Failed to deserialize RootSliceEntity";
            delete canvasEntity;
            return false;
        }

        // --- Step 3: Find SliceComponent ---
        auto* sliceComponent = rootSliceEntity->FindComponent<AZ::SliceComponent>();
        if (!sliceComponent)
        {
            outError = "No SliceComponent found on RootSliceEntity";
            delete rootSliceEntity;
            delete canvasEntity;
            return false;
        }

        // Allow partial instantiation so we still get entities even if some slice refs fail
        sliceComponent->AllowPartialInstantiation(true);
        sliceComponent->SetSerializeContext(serializeContext);

        // --- Step 4: Get entities and slice references separately ---
        // First trigger instantiation by calling GetEntities (we discard the merged result)
        {
            AZ::SliceComponent::EntityList allEntities;
            sliceComponent->GetEntities(allEntities);
        }

        // Now get direct entities (not from any reference)
        const AZ::SliceComponent::EntityList& directEntities = sliceComponent->GetNewEntities();

        // --- Step 5: Build UiCanvasFileObject and serialize ---
        UiCanvasFileObject fileObject;
        fileObject.m_canvasEntity = canvasEntity;

        // Add direct entities to ChildEntities
        for (AZ::Entity* entity : directEntities)
        {
            fileObject.m_childEntities.push_back(entity);
        }

        // Convert SliceReferences to UiPrefabInstance objects
        const AZ::SliceComponent::SliceList& sliceRefs = sliceComponent->GetSlices();

        for (const AZ::SliceComponent::SliceReference& sliceRef : sliceRefs)
        {
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset = sliceRef.GetSliceAsset();
            AZStd::string sliceAssetHint = sliceAsset.GetHint();

            // Convert .slice path to .uiprefab path
            AZStd::string uiprefabPath = sliceAssetHint;
            AZ::StringFunc::Replace(uiprefabPath, ".slice", ".uiprefab");

            const auto& instances = sliceRef.GetInstances();
            for (const AZ::SliceComponent::SliceInstance& instance : instances)
            {
                const auto* instantiated = instance.GetInstantiated();
                if (!instantiated || instantiated->m_entities.empty())
                {
                    AZ_Warning("LYShineToShine", false, "SliceReference instance not instantiated in %s", filePath.c_str());
                    continue;
                }

                // Add instantiated entities to ChildEntities -- they already have
                // correct parent-child relationships and canvas-specific entity IDs.
                // PrefabInstances are stored as metadata for future save/reload support
                // but entities are NOT re-instantiated from .uiprefab at load time
                // because the entity IDs in .uiprefab don't match the entityIdMap
                // (ConvertSliceToUiPrefab generates different IDs than the slice template).
                for (AZ::Entity* entity : instantiated->m_entities)
                {
                    fileObject.m_childEntities.push_back(entity);
                }

                // Build UiPrefabInstance (metadata only for now)
                UiPrefabInstance prefabInstance;
                prefabInstance.m_sourcePath = uiprefabPath;
                prefabInstance.m_instanceId = instance.GetId();
                for (const auto& [baseId, instanceEntityId] : instance.GetEntityIdMap())
                {
                    prefabInstance.m_entityIdMap[baseId] = instanceEntityId;
                }
                // PatchesJson empty for now
                fileObject.m_prefabInstances.push_back(AZStd::move(prefabInstance));
            }
        }

        // --- Step 6: Serialize via SaveObjectToStream ---
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> byteStream(&byteBuffer);
        UiCanvasFileObject::SaveCanvasToStream(byteStream, &fileObject);

        AZ::IO::SystemFile outFile;
        if (!outFile.Open(filePath.c_str(),
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            outError = AZStd::string::format("Failed to open file for writing: %s", filePath.c_str());
            fileObject.m_canvasEntity = nullptr;
            fileObject.m_childEntities.clear();
            delete rootSliceEntity;
            delete canvasEntity;
            return false;
        }

        outFile.Write(byteBuffer.data(), byteBuffer.size());
        outFile.Close();

        AZ_TracePrintf("LYShineToShine", "  Upgraded: %s (%zu direct entities, %zu prefab instances)\n",
            filePath.c_str(), directEntities.size(), fileObject.m_prefabInstances.size());

        // Don't let fileObject destructor delete entities we don't own
        fileObject.m_canvasEntity = nullptr;
        fileObject.m_childEntities.clear();

        // rootSliceEntity owns the instantiated entities via SliceComponent
        delete rootSliceEntity;
        // The canvas entity is owned by this function, not by the root slice entity.
        delete canvasEntity;
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Slice-to-UiPrefab conversion
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    CanvasUpgrader::SliceConvertReport CanvasUpgrader::ConvertSlicesInDirectory(const AZ::IO::Path& directoryPath)
    {
        SliceConvertReport report;

        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            report.m_failureDetails.emplace_back("(system)", "FileIO not available");
            return report;
        }

        // Recursively find all .slice files
        AZStd::vector<AZ::IO::Path> sliceFiles;
        fileIO->FindFiles(directoryPath.c_str(), "*.slice",
            [&sliceFiles](const char* filePath) -> bool
            {
                sliceFiles.emplace_back(filePath);
                return true;
            });

        // Also search subdirectories
        AZStd::function<void(const AZ::IO::Path&)> searchSubdirs;
        searchSubdirs = [&](const AZ::IO::Path& dir)
        {
            fileIO->FindFiles(dir.c_str(), "*",
                [&](const char* entry) -> bool
                {
                    AZ::IO::Path entryPath(entry);
                    if (fileIO->IsDirectory(entryPath.c_str()))
                    {
                        fileIO->FindFiles(entryPath.c_str(), "*.slice",
                            [&sliceFiles](const char* filePath) -> bool
                            {
                                sliceFiles.emplace_back(filePath);
                                return true;
                            });
                        searchSubdirs(entryPath);
                    }
                    return true;
                });
        };
        searchSubdirs(directoryPath);

        AZ_TracePrintf("LYShineToShine", "Found %zu .slice files\n", sliceFiles.size());

        for (const auto& filePath : sliceFiles)
        {
            report.m_filesScanned++;
            AZStd::string error;
            if (ConvertSliceToUiPrefab(filePath, error))
            {
                report.m_converted++;
            }
            else if (error == "Not a UI slice (no UiElementComponent found)")
            {
                report.m_skippedNonUi++;
            }
            else
            {
                report.m_failed++;
                report.m_failureDetails.push_back({ filePath.String(), error });
            }
        }

        return report;
    }

    bool CanvasUpgrader::ConvertSliceToUiPrefab(const AZ::IO::Path& slicePath, AZStd::string& outError)
    {
        AZ_TracePrintf("LYShineToShine", "Converting slice to uiprefab: %s\n", slicePath.c_str());

        // Resolve alias path to absolute path
        AZ::IO::Path resolvedPath = slicePath;
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            char resolvedBuffer[AZ::IO::MaxPathLength];
            if (fileIO->ResolvePath(slicePath.c_str(), resolvedBuffer, AZ::IO::MaxPathLength))
            {
                resolvedPath = resolvedBuffer;
            }
        }

        // Read the .slice file
        AZ::IO::SystemFile file;
        if (!file.Open(resolvedPath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            outError = AZStd::string::format("Failed to open file: %s", resolvedPath.c_str());
            return false;
        }

        AZ::IO::SizeType fileSize = file.Length();
        AZStd::string xmlContent;
        xmlContent.resize_no_construct(fileSize);
        file.Read(fileSize, xmlContent.data());
        file.Close();

        // The .slice file is an ObjectStream XML containing a root AZ::Entity with a SliceComponent
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            outError = "SerializeContext not available";
            return false;
        }

        // Deserialize the root entity (which contains the SliceComponent)
        AZ::IO::MemoryStream sliceStream(xmlContent.data(), xmlContent.size());
        AZ::Entity* rootEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(
            sliceStream, serializeContext,
            AZ::ObjectStream::FilterDescriptor(&AZ::ObjectStream::AssetFilterSlicesOnly));
        if (!rootEntity)
        {
            outError = "Failed to deserialize slice root entity";
            return false;
        }

        // Find the SliceComponent
        auto* sliceComponent = rootEntity->FindComponent<AZ::SliceComponent>();
        if (!sliceComponent)
        {
            outError = "No SliceComponent found in slice file";
            delete rootEntity;
            return false;
        }

        sliceComponent->AllowPartialInstantiation(true);
        sliceComponent->SetSerializeContext(serializeContext);

        // Get all entities (triggers instantiation of nested slice references)
        AZ::SliceComponent::EntityList entities;
        bool getEntitiesResult = sliceComponent->GetEntities(entities);

        if (entities.empty())
        {
            if (!getEntitiesResult)
            {
                outError = "SliceComponent::GetEntities() failed and no entities available";
            }
            else
            {
                outError = "No entities found in slice";
            }
            delete rootEntity;
            return false;
        }

        if (!getEntitiesResult)
        {
            AZ_Warning("LYShineToShine", false,
                "GetEntities() returned false for %s but %zu entities were recovered (partial instantiation)",
                slicePath.c_str(), entities.size());
        }

        // Check if this is a UI slice by looking for UiElementComponent on any entity.
        // Non-UI slices (level slices, entity slices, etc.) are skipped.
        static const AZ::Uuid uiElementComponentUuid("{4A97D63E-CE7A-45B6-AAE4-102DB4334688}");
        bool isUiSlice = false;
        for (AZ::Entity* entity : entities)
        {
            if (entity->FindComponent(uiElementComponentUuid))
            {
                isUiSlice = true;
                break;
            }
        }
        if (!isUiSlice)
        {
            outError = "Not a UI slice (no UiElementComponent found)";
            delete rootEntity;
            return false;
        }

        // Build a set of all entity IDs for parent lookup
        AZStd::unordered_set<AZ::EntityId> entityIdSet;
        for (AZ::Entity* entity : entities)
        {
            entityIdSet.insert(entity->GetId());
        }

        // Serialize all entities to a JSON document.
        // Format: { "Entities": { "Entity_[id]": { ... }, ... } }
        // This matches the engine prefab format and enables JSON Patch overrides.
        rapidjson::Document prefabDom(rapidjson::kObjectType);
        auto& allocator = prefabDom.GetAllocator();

        rapidjson::Value entitiesObj(rapidjson::kObjectType);

        AZ::JsonRegistrationContext* jsonRegistrationContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            jsonRegistrationContext, &AZ::ComponentApplicationBus::Events::GetJsonRegistrationContext);

        for (AZ::Entity* entity : entities)
        {
            rapidjson::Value entityValue(rapidjson::kObjectType);

            AZ::JsonSerializerSettings serializerSettings;
            serializerSettings.m_serializeContext = serializeContext;
            serializerSettings.m_registrationContext = jsonRegistrationContext;

            AZ::JsonSerializationResult::ResultCode storeResult =
                AZ::JsonSerialization::Store(entityValue, allocator, *entity, serializerSettings);

            if (storeResult.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted)
            {
                // Use Entity_[id] as the key, matching engine prefab format
                AZStd::string entityAlias = AZStd::string::format("Entity_[%llu]",
                    static_cast<AZ::u64>(entity->GetId()));
                rapidjson::Value key(entityAlias.c_str(), allocator);
                entitiesObj.AddMember(key, entityValue, allocator);
            }
            else
            {
                AZ_Warning("LYShineToShine", false, "Failed to serialize entity '%s' to JSON",
                    entity->GetName().c_str());
            }
        }

        prefabDom.AddMember("Entities", entitiesObj, allocator);

        // Write JSON to string
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 4);
        prefabDom.Accept(writer);

        // Write the .uiprefab file alongside the .slice
        AZ::IO::Path outputPath = resolvedPath;
        outputPath.ReplaceExtension("uiprefab");

        AZ::IO::SystemFile outFile;
        if (!outFile.Open(outputPath.c_str(),
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH))
        {
            outError = AZStd::string::format("Failed to create output file: %s", outputPath.c_str());
            delete rootEntity;
            return false;
        }

        outFile.Write(buffer.GetString(), buffer.GetSize());
        outFile.Close();

        AZ_TracePrintf("LYShineToShine", "  Converted: %s -> %s (%zu entities)\n",
            slicePath.c_str(), outputPath.c_str(), entities.size());

        delete rootEntity;
        return true;
    }

} // namespace LYShineToShine
