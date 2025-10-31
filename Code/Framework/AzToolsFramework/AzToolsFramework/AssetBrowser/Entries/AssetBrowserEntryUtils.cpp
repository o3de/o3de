/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>

#include <QMimeData>
#include <QUrl>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        namespace Utils
        {
            void ToMimeData(QMimeData* mimeData, const AZStd::vector<const AssetBrowserEntry*>& entries)
            {
                if (!mimeData)
                {
                    return;
                }

                if (entries.empty())
                {
                    return;
                }

                AZStd::string outputString = ToString(entries);

                // When constructing a QMimeData object, it is unknown what other applications will use it for.
                // Other applications will examine the object and based on what different types of data are packed
                // into it, take appropriate action.  In general, its better to pack whatever information
                // other applications might need in order to make decisions.
                // For this purpose, pack them as special AssetBrowserEntry objects (in case it is dropped into
                // another window in this application or another O3DE application that understands these objects),
                // but also pack them as Plain Text (so you could drag it into, say, a text editor and get something),
                // and also as URLs to the files (so that you could drag an image into, say, an image editing tool
                // and have the image editor tool understand and load the files.
                // Without the URLs, other applications will not understand that these entries represent files on disk.
                QList<QUrl> urls;

                for (const AssetBrowserEntry* entry : entries)
                {
                    if (entry)
                    {
                        // entries may have aliased paths:
                        AZStd::string entryPath = entry->GetFullPath();
                        if (AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance())
                        {
                            AZ::IO::FixedMaxPath fixedPath;
                            if (fileIO->ResolvePath(fixedPath, entryPath.c_str()))
                            {
                                entryPath = fixedPath.LexicallyNormal().String();
                            }
                        }

                        urls.push_back(QUrl::fromLocalFile(QString::fromUtf8(entryPath.c_str())));
                    }
                }
                // Saving the buffer of asset browser entries back to the mime data
                QByteArray byteArray(outputString.c_str(), static_cast<int>(outputString.size()));
                mimeData->setData(AssetBrowserEntry::GetMimeType(), byteArray);
                mimeData->setData("text/plain", byteArray);
                mimeData->setUrls(urls);
            }

            AZStd::string ToString(const AZStd::vector<const AssetBrowserEntry*>& entries)
            {
                if (entries.empty())
                {
                    return AZStd::string();
                }

                AZStd::string outputString;
                outputString.reserve(entries.size() * 64); // roughly average size of uuid or assetids.

                // note that due to the way Qt and mimedata works, its quite possible for duplicates
                // to be in the entry list.  Intead of requiring every single caller de-duplicate, we will
                // deduplicate here, in one place.
                AZStd::unordered_set<const AssetBrowserEntry*> alreadyAdded;

                for (const AssetBrowserEntry* entry : entries)
                {
                    if (!entry || alreadyAdded.contains(entry))
                    {
                        continue;
                    }

                    AZStd::string outString = WriteEntryToString(entry);
                    if (outString.empty())
                    {
                        continue;
                    }
                    outputString.append(outString);
                    outputString.append("\n");
                    alreadyAdded.insert(entry);
                }
                return outputString;
            }

            bool FromString(AZStd::string_view inputString, AZStd::vector<const AssetBrowserEntry*>& entries)
            {
                AZStd::unordered_set<const AssetBrowserEntry*> alreadyAdded;

                bool anyFound = false;
                size_t newline_pos = inputString.find('\n', 0);
                while (newline_pos != AZStd::string_view::npos)
                {
                    AZStd::string_view subLine(inputString.begin(), newline_pos);
                    const AssetBrowserEntry* target = FindFromString(subLine);
                    if (target)
                    {
                        anyFound = true;
                        if (!alreadyAdded.contains(target))
                        {
                            entries.push_back(target);
                            alreadyAdded.insert(target);
                        }
                    }
                    inputString = AZStd::string_view(inputString.begin() + newline_pos + 1, inputString.end());
                    newline_pos = inputString.find('\n', 0);
                }
                return anyFound;
            }

            bool FromMimeData(const QMimeData* mimeData, AZStd::vector<const AssetBrowserEntry*>& entries)
            {
                if (!mimeData)
                {
                    return false;
                }

                if (!mimeData->hasFormat(AssetBrowserEntry::GetMimeType()))
                {
                    return false;
                }

                QByteArray byteData = mimeData->data(AssetBrowserEntry::GetMimeType());
                return FromString(AZStd::string_view(byteData.data(), byteData.size()), entries);
            }

            AZStd::string WriteEntryToString(const AssetBrowserEntry* entry)
            {
                if (!entry)
                {
                    return AZStd::string();
                }
                // we only currently support drag and drop on products, and sources, not roots, or folders.
                // to support folders (if it is ever necessary, it would be necessary to add a way to write a stable
                // identifier for folders into the AssetBrowserEntryCache, something that can be used to look up a folder
                // and then write that folder in here.  Absolute path would work, but is not portable (ie, one user copying AB entries
                // to another user on a different machine).
                AZStd::string entryTypeString = AssetBrowserEntry::AssetEntryTypeToString(entry->GetEntryType()).toUtf8().constData();
                switch (entry->GetEntryType())
                {
                    case AssetBrowserEntry::AssetEntryType::Source:
                        return AZStd::string::format(
                            "%s|%s//%s", // "Source|{SOME GUID}//friendly name
                            entryTypeString.c_str(),
                            static_cast<const SourceAssetBrowserEntry*>(entry)->GetSourceUuid().ToString<AZStd::string>().c_str(),
                            entry->GetName().c_str());
                        break;
                    case AssetBrowserEntry::AssetEntryType::Product:
                        return AZStd::string::format(
                            "%s|%s//%s", // "Product|{SOME ASSETID}//friendly name hint
                            entryTypeString.c_str(),
                            static_cast<const ProductAssetBrowserEntry*>(entry)->GetAssetId().ToString<AZStd::string>().c_str(),
                            entry->GetName().c_str());
                        break;
                    case AssetBrowserEntry::AssetEntryType::Folder:
                        return AZStd::string::format(
                            "%s|%s//%s", // "Folder|{SOME ASSETID}//friendly name hint
                            entryTypeString.c_str(),
                            static_cast<const FolderAssetBrowserEntry*>(entry)->GetFolderUuid().ToString<AZStd::string>().c_str(),
                            entry->GetName().c_str());
                        break;
                    default:
                        AZ_Warning("Asset Browser", false, "Asset browser does not support this operation on folders.");
                        break;
                }

                return AZStd::string();
            }

            const AssetBrowserEntry* FindFromString(AZStd::string_view data)
            {
                size_t split = data.find('|', 0);
                if (split == AZStd::string_view::npos)
                {
                    return nullptr;
                }
                AZStd::string_view typeNamePortion(data.begin(), data.begin() + split);
                AZStd::string_view dataPortion(data.begin() + split + 1, data.end());
                // the data portion is allowed to optionally have // in it, if it does, it is considered
                // unused data.
                size_t dataPortionCommentBegins = dataPortion.find("//");
                if (dataPortionCommentBegins != AZStd::string_view::npos)
                {
                    dataPortion = AZStd::string_view(dataPortion.begin(), dataPortion.begin() + dataPortionCommentBegins);
                }

                if (typeNamePortion.compare(
                        AssetBrowserEntry::AssetEntryTypeToString(AssetBrowserEntry::AssetEntryType::Source).toUtf8().constData()) == 0)
                {
                    // the data is a source uuid.
                    AZ::Uuid sourceUuid = AZ::Uuid::CreateString(dataPortion.data(), dataPortion.size());
                    return SourceAssetBrowserEntry::GetSourceByUuid(sourceUuid);
                }
                else if (typeNamePortion.compare(
                    AssetBrowserEntry::AssetEntryTypeToString(AssetBrowserEntry::AssetEntryType::Product).toUtf8().constData()) == 0)
                {
                    // the data is an asset id.
                    AZ::Data::AssetId assetId = AZ::Data::AssetId::CreateString(dataPortion);
                    return ProductAssetBrowserEntry::GetProductByAssetId(assetId);
                }
                else if (typeNamePortion.compare(
                        AssetBrowserEntry::AssetEntryTypeToString(AssetBrowserEntry::AssetEntryType::Folder).toUtf8().constData()) == 0)
                {
                    // the data is an folder uuid.
                    AZ::Uuid folderUuid = AZ::Uuid::CreateString(dataPortion.data(), dataPortion.size());
                    return FolderAssetBrowserEntry::GetFolderByUuid(folderUuid);
                }
                else
                {
                    AZ_Error("Asset Browser", false, "Warning, invalid data for asset browser decode, ignored: %.*s", data.length(), data.data());
                }
                return nullptr;
            }

            const AssetBrowserEntry* FolderForEntry(const AssetBrowserEntry* entry)
            {
                while (entry && entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Folder)
                {
                    entry = entry->GetParent();
                }
                return entry;
            }
        } // namespace Utils
    } // namespace AssetBrowser
} // namespace AzToolsFramework
