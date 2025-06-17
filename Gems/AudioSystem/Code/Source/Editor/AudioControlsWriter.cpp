/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControlsWriter.h>

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/XML/rapidxml_print.h>

#include <ACEEnums.h>
#include <ATLControlsModel.h>
#include <IAudioSystem.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>

#include <IEditor.h>
#include <Include/IFileUtil.h>

#include <QModelIndex>
#include <QStandardItemModel>
#include <QFileInfo>


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

            auto fileIO = AZ::IO::FileIOBase::GetInstance();
            AZStd::for_each(
                m_foundLibraryPaths.begin(), m_foundLibraryPaths.end(),
                [fileIO](AZStd::string& libraryPath) -> void
                {
                    if (auto newPathOpt = fileIO->ConvertToAlias(AZ::IO::PathView{ libraryPath });
                        newPathOpt.has_value())
                    {
                        libraryPath = newPathOpt.value().Native();
                    }
                    AZStd::to_lower(libraryPath.begin(), libraryPath.end());
                });

            // Delete libraries that don't exist anymore from disk
            FilepathSet librariesToDelete;
            AZStd::set_difference(
                previousLibraryPaths.begin(), previousLibraryPaths.end(),
                m_foundLibraryPaths.begin(), m_foundLibraryPaths.end(),
                AZStd::inserter(librariesToDelete, librariesToDelete.begin())
            );

            for (auto it = librariesToDelete.begin(); it != librariesToDelete.end(); ++it)
            {
                auto newPathOpt = fileIO->ResolvePath(AZ::IO::PathView{ *it });
                DeleteLibraryFile(newPathOpt.value().Native());
            }

            previousLibraryPaths = m_foundLibraryPaths;

            m_layoutModel->blockSignals(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteLibrary(const AZStd::string_view libraryName, QModelIndex root)
    {
        const char* controlsPath = AZ::Interface<Audio::IAudioSystem>::Get()->GetControlsPath();

        if (root.isValid() && controlsPath)
        {
            TLibraryStorage library;
            int i = 0;
            QModelIndex child = root.model()->index(i, 0, root);
            while (child.isValid())
            {
                WriteItem(child, "", library, root.data(eDR_MODIFIED).toBool());
                child = root.model()->index(++i, 0, root);
            }

            for (auto& libraryPair : library)
            {
                AZ::IO::FixedMaxPath libraryPath{ controlsPath };
                const AZStd::string& scope = libraryPair.first;
                if (scope.empty())
                {
                    // no scope, file at the root level
                    libraryPath /= libraryName;
                    libraryPath.ReplaceExtension(WriterStrings::LibraryExtension);
                }
                else
                {
                    // with scope, inside level folder
                    libraryPath /= AZ::IO::FixedMaxPath{ WriterStrings::LevelsSubFolder } / scope / libraryName;
                    libraryPath.ReplaceExtension(WriterStrings::LibraryExtension);
                }

                AZ::IO::FixedMaxPath fullFilePath = AZ::Utils::GetProjectPath();
                fullFilePath /= libraryPath;
                m_foundLibraryPaths.insert(fullFilePath.c_str());

                const SLibraryScope& libScope = libraryPair.second;
                if (libScope.m_isDirty)
                {
                    XmlAllocator& xmlAlloc(AudioControls::s_xmlAllocator);
                    AZ::rapidxml::xml_node<char>* fileNode =
                        xmlAlloc.allocate_node(AZ::rapidxml::node_element, xmlAlloc.allocate_string(Audio::ATLXmlTags::RootNodeTag));

                    AZ::rapidxml::xml_attribute<char>* nameAttr = xmlAlloc.allocate_attribute(
                        xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLNameAttribute), xmlAlloc.allocate_string(libraryName.data()));

                    fileNode->append_attribute(nameAttr);

                    for (int ii = 0; ii < eACET_NUM_TYPES; ++ii)
                    {
                        if (libScope.m_nodes[ii] && libScope.m_nodes[ii]->first_node() != nullptr)
                        {
                            fileNode->append_node(libScope.m_nodes[ii]);
                        }
                    }

                    if (auto fileInfo = QFileInfo(fullFilePath.c_str());
                        fileInfo.exists())
                    {
                        if (!fileInfo.isWritable())
                        {
                            // file exists and is read-only
                            CheckOutFile(fullFilePath.Native());
                        }

                        [[maybe_unused]] bool writeOk = WriteXmlToFile(fullFilePath.Native(), fileNode);
                    }
                    else
                    {
                        // since it's a new file, save the file first, CheckOutFile will add it
                        [[maybe_unused]] bool writeOk = WriteXmlToFile(fullFilePath.Native(), fileNode);
                        CheckOutFile(fullFilePath.Native());
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
    bool CAudioControlsWriter::WriteXmlToFile(const AZStd::string_view filepath, AZ::rapidxml::xml_node<char>* rootNode)
    {
        if (!rootNode)
        {
            return false;
        }

        using namespace AZ::IO;
        AZStd::string docString;
        ByteContainerStream stringStream(&docString);

        AZ::rapidxml::xml_document<char> xmlDoc;
        xmlDoc.append_node(rootNode);

        RapidXMLStreamWriter streamWriter(&stringStream);
        AZ::rapidxml::print(streamWriter.Iterator(), xmlDoc);
        streamWriter.FlushCache();

        constexpr int openMode =
            (SystemFile::SF_OPEN_WRITE_ONLY | SystemFile::SF_OPEN_CREATE | SystemFile::SF_OPEN_CREATE_PATH);

        if (SystemFile fileOut;
            fileOut.Open(filepath.data(), openMode))
        {
            auto bytesWritten = fileOut.Write(docString.data(), docString.size());
            return (bytesWritten == docString.size());
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteControlToXml(AZ::rapidxml::xml_node<char>* node, CATLControl* control, const AZStd::string_view path)
    {
        if (!node || !control)
        {
            return;
        }

        XmlAllocator& xmlAlloc(AudioControls::s_xmlAllocator);

        const EACEControlType type = control->GetType();
        AZStd::string_view typeName = TypeToTag(type);

        AZ::rapidxml::xml_node<char>* childNode =
            xmlAlloc.allocate_node(AZ::rapidxml::node_element, xmlAlloc.allocate_string(typeName.data()));

        AZ::rapidxml::xml_attribute<char>* nameAttr = xmlAlloc.allocate_attribute(
            xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLNameAttribute), xmlAlloc.allocate_string(control->GetName().c_str()));

        childNode->append_attribute(nameAttr);

        if (!path.empty())
        {
            AZ::rapidxml::xml_attribute<char>* pathAttr = xmlAlloc.allocate_attribute(
                xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLPathAttribute), xmlAlloc.allocate_string(path.data()));

            childNode->append_attribute(pathAttr);
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
                AZ::rapidxml::xml_attribute<char>* loadAttr = xmlAlloc.allocate_attribute(
                    xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLTypeAttribute),
                    xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLDataLoadType));

                childNode->append_attribute(loadAttr);
            }

            // New Preloads XML...
            WriteConnectionsToXml(childNode, control);
        }
        else
        {
            WriteConnectionsToXml(childNode, control);
        }

        node->append_node(childNode);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteConnectionsToXml(AZ::rapidxml::xml_node<char>* node, CATLControl* control)
    {
        if (node && control && m_audioSystemImpl)
        {
            for (auto& connectionNode : control->m_connectionNodes)
            {
                if (!connectionNode.m_isValid)
                {
                    XmlAllocator& xmlAlloc(AudioControls::s_xmlAllocator);
                    node->append_node(xmlAlloc.clone_node(connectionNode.m_xmlNode));
                }
            }

            const size_t size = control->ConnectionCount();
            for (size_t i = 0; i < size; ++i)
            {
                if (TConnectionPtr connection = control->GetConnectionAt(i);
                    connection != nullptr)
                {
                    if (auto childNode = m_audioSystemImpl->CreateXMLNodeFromConnection(connection, control->GetType());
                        childNode != nullptr)
                    {
                        node->append_node(childNode);
                        control->m_connectionNodes.emplace_back(childNode, true);
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::CheckOutFile(const AZStd::string_view filepath)
    {
        IEditor* editor = GetIEditor();
        IFileUtil* fileUtil = editor ? editor->GetFileUtil() : nullptr;
        if (fileUtil)
        {
            fileUtil->CheckoutFile(AZ::IO::FixedMaxPath{ filepath }.c_str(), nullptr);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::DeleteLibraryFile(const AZStd::string_view filepath)
    {
        IEditor* editor = GetIEditor();
        IFileUtil* fileUtil = editor ? editor->GetFileUtil() : nullptr;
        if (fileUtil)
        {
            fileUtil->DeleteFromSourceControl(AZ::IO::FixedMaxPath{ filepath }.c_str(), nullptr);
        }
    }

} // namespace AudioControls
