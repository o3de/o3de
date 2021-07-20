/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControlsWriter.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <ACEEnums.h>
#include <ATLControlsModel.h>
#include <CryFile.h>
#include <IAudioSystem.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <IEditor.h>
#include <Include/IFileUtil.h>
#include <Include/ISourceControl.h>
#include <ISystem.h>
#include <StringUtils.h>
#include <Util/PathUtil.h>

#include <QModelIndex>
#include <QStandardItemModel>
#include <QFileInfo>


using namespace PathUtil;

namespace AudioControls
{
    namespace WriterStrings
    {
        static constexpr const char* LevelsSubFolder = "levels";
        static constexpr const char* LibraryExtension = ".xml";

    } // namespace WriterStrings

    //-------------------------------------------------------------------------------------------//
    AZStd::string_view TypeToTag(EACEControlType type)
    {
        switch (type)
        {
        case eACET_RTPC:
            return Audio::ATLXmlTags::ATLRtpcTag;
        case eACET_TRIGGER:
            return Audio::ATLXmlTags::ATLTriggerTag;
        case eACET_SWITCH:
            return Audio::ATLXmlTags::ATLSwitchTag;
        case eACET_SWITCH_STATE:
            return Audio::ATLXmlTags::ATLSwitchStateTag;
        case eACET_PRELOAD:
            return Audio::ATLXmlTags::ATLPreloadRequestTag;
        case eACET_ENVIRONMENT:
            return Audio::ATLXmlTags::ATLEnvironmentTag;
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    CAudioControlsWriter::CAudioControlsWriter(CATLControlsModel* atlModel, QStandardItemModel* layoutModel, IAudioSystemEditor* audioSystemImpl, FilepathSet& previousLibraryPaths)
        : m_atlModel(atlModel)
        , m_layoutModel(layoutModel)
        , m_audioSystemImpl(audioSystemImpl)
    {
        if (m_atlModel && m_layoutModel && m_audioSystemImpl)
        {
            m_layoutModel->blockSignals(true);

            int i = 0;
            QModelIndex index = m_layoutModel->index(i, 0);
            while (index.isValid())
            {
                WriteLibrary(index.data(Qt::DisplayRole).toString().toUtf8().data(), index);
                index = index.sibling(++i, 0);
            }


            // Delete libraries that don't exist anymore from disk
            FilepathSet librariesToDelete;
            AZStd::set_difference(
                previousLibraryPaths.begin(), previousLibraryPaths.end(),
                m_foundLibraryPaths.begin(), m_foundLibraryPaths.end(),
                AZStd::inserter(librariesToDelete, librariesToDelete.begin())
            );

            for (auto it = librariesToDelete.begin(); it != librariesToDelete.end(); ++it)
            {
                DeleteLibraryFile((*it).c_str());
            }

            previousLibraryPaths = m_foundLibraryPaths;

            m_layoutModel->blockSignals(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteLibrary(const AZStd::string_view libraryName, QModelIndex root)
    {
        if (root.isValid())
        {
            TLibraryStorage library;
            int i = 0;
            QModelIndex child = root.model()->index(i, 0, root);
            while (child.isValid())
            {
                WriteItem(child, "", library, root.data(eDR_MODIFIED).toBool());
                child = root.model()->index(++i, 0, root);
            }

            const char* controlsPath = nullptr;
            Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);

            for (auto& libraryPair : library)
            {
                AZStd::string libraryPath;
                const AZStd::string& scope = libraryPair.first;
                if (scope.empty())
                {
                    // no scope, file at the root level
                    libraryPath.append(controlsPath);
                    AZ::StringFunc::Path::Join(libraryPath.c_str(), libraryName.data(), libraryPath);
                    libraryPath.append(WriterStrings::LibraryExtension);
                }
                else
                {
                    // with scope, inside level folder
                    libraryPath.append(controlsPath);
                    libraryPath.append(WriterStrings::LevelsSubFolder);
                    AZ::StringFunc::Path::Join(libraryPath.c_str(), scope.c_str(), libraryPath);
                    AZ::StringFunc::Path::Join(libraryPath.c_str(), libraryName.data(), libraryPath);
                    libraryPath.append(WriterStrings::LibraryExtension);
                }

                // should be able to change this back to GamePathToFullPath once a path normalization bug has been fixed:
                AZStd::string fullFilePath;
                AZ::StringFunc::Path::Join(Path::GetEditingGameDataFolder().c_str(), libraryPath.c_str(), fullFilePath);
                AZStd::to_lower(fullFilePath.begin(), fullFilePath.end());
                m_foundLibraryPaths.insert(fullFilePath.c_str());

                const SLibraryScope& libScope = libraryPair.second;
                if (libScope.m_isDirty)
                {
                    XmlNodeRef fileNode = GetISystem()->CreateXmlNode(Audio::ATLXmlTags::RootNodeTag);
                    fileNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, libraryName.data());

                    for (int ii = 0; ii < eACET_NUM_TYPES; ++ii)
                    {
                        if (ii != eACET_SWITCH_STATE) // switch_states are written inside the switches
                        {
                            if (libScope.m_nodes[ii]->getChildCount() > 0)
                            {
                                fileNode->addChild(libScope.m_nodes[ii]);
                            }
                        }
                    }

                    if (QFileInfo::exists(fullFilePath.c_str()))
                    {
                        const DWORD fileAttributes = GetFileAttributes(fullFilePath.c_str());
                        if (fileAttributes & FILE_ATTRIBUTE_READONLY)
                        {
                            // file is read-only
                            CheckOutFile(fullFilePath);
                        }
                        fileNode->saveToFile(fullFilePath.c_str());
                    }
                    else
                    {
                        // save the file, CheckOutFile will add it, since it's new
                        fileNode->saveToFile(fullFilePath.c_str());
                        CheckOutFile(fullFilePath);
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteItem(QModelIndex index, const AZStd::string& path, TLibraryStorage& library, bool isParentModified)
    {
        if (index.isValid())
        {
            if (index.data(eDR_TYPE) == eIT_FOLDER)
            {
                int i = 0;
                QModelIndex child = index.model()->index(i, 0, index);
                while (child.isValid())
                {
                    AZStd::string newPath = path.empty() ? "" : path + "/";
                    newPath += index.data(Qt::DisplayRole).toString().toUtf8().data();
                    WriteItem(child, newPath, library, index.data(eDR_MODIFIED).toBool() || isParentModified);
                    child = index.model()->index(++i, 0, index);
                }

                QStandardItem* item = m_layoutModel->itemFromIndex(index);
                if (item)
                {
                    item->setData(false, eDR_MODIFIED);
                }
            }
            else
            {
                CATLControl* control = m_atlModel->GetControlByID(index.data(eDR_ID).toUInt());
                if (control)
                {
                    SLibraryScope& scope = library[control->GetScope()];
                    if (IsItemModified(index) || isParentModified)
                    {
                        scope.m_isDirty = true;
                        QStandardItem* item = m_layoutModel->itemFromIndex(index);
                        if (item)
                        {
                            item->setData(false, eDR_MODIFIED);
                        }
                    }

                    WriteControlToXml(scope.m_nodes[control->GetType()], control, path);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    bool CAudioControlsWriter::IsItemModified(QModelIndex index)
    {
        if (index.data(eDR_MODIFIED).toBool() == true)
        {
            return true;
        }

        int i = 0;
        QModelIndex child = index.model()->index(i, 0, index);
        while (child.isValid())
        {
            if (IsItemModified(child))
            {
                return true;
            }
            child = index.model()->index(++i, 0, index);
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteControlToXml(XmlNodeRef node, CATLControl* control, const AZStd::string_view path)
    {
        const EACEControlType type = control->GetType();
        XmlNodeRef childNode = node->createNode(TypeToTag(type).data());
        childNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, control->GetName().c_str());
        if (!path.empty())
        {
            childNode->setAttr("path", path.data());
        }

        if (type == eACET_SWITCH)
        {
            const size_t size = control->ChildCount();
            for (size_t i = 0; i < size; ++i)
            {
                WriteControlToXml(childNode, control->GetChild(i), "");
            }
        }
        else if (type == eACET_PRELOAD)
        {
            if (control->IsAutoLoad())
            {
                childNode->setAttr(Audio::ATLXmlTags::ATLTypeAttribute, Audio::ATLXmlTags::ATLDataLoadType);
            }

            // New Preloads XML...
            WriteConnectionsToXml(childNode, control);
        }
        else
        {
            WriteConnectionsToXml(childNode, control);
        }

        node->addChild(childNode);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteConnectionsToXml(XmlNodeRef node, CATLControl* control)
    {
        if (control && m_audioSystemImpl)
        {
            TXmlNodeList otherNodes = control->m_connectionNodes;
            auto end = AZStd::remove_if(otherNodes.begin(), otherNodes.end(),
                [](const SRawConnectionData& node)
                {
                    return node.m_isValid;
                }
            );
            otherNodes.erase(end, otherNodes.end());

            for (auto& connectionNode : otherNodes)
            {
                node->addChild(connectionNode.m_xmlNode);
            }

            const size_t size = control->ConnectionCount();
            for (size_t i = 0; i < size; ++i)
            {
                TConnectionPtr connection = control->GetConnectionAt(i);
                if (connection)
                {
                    XmlNodeRef childNode = m_audioSystemImpl->CreateXMLNodeFromConnection(connection, control->GetType());
                    if (childNode)
                    {
                        node->addChild(childNode);
                        control->m_connectionNodes.push_back(SRawConnectionData(childNode, true));
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::CheckOutFile(const AZStd::string& filepath)
    {
        IEditor* editor = GetIEditor();
        IFileUtil* fileUtil = editor ? editor->GetFileUtil() : nullptr;
        if (fileUtil)
        {
            fileUtil->CheckoutFile(filepath.c_str(), nullptr);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::DeleteLibraryFile(const AZStd::string& filepath)
    {
        IEditor* editor = GetIEditor();
        IFileUtil* fileUtil = editor ? editor->GetFileUtil() : nullptr;
        if (fileUtil)
        {
            fileUtil->DeleteFromSourceControl(filepath.c_str(), nullptr);
        }
    }

} // namespace AudioControls
