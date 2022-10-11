/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControlsLoader.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>

#include <ACEEnums.h>
#include <ATLCommon.h>
#include <ATLControlsModel.h>
#include <AudioFileUtils.h>
#include <IAudioSystem.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <QAudioControlTreeWidget.h>

#include <Util/UndoUtil.h>

#include <QStandardItem>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    namespace LoaderStrings
    {
        static constexpr const char* LevelsSubFolder = "levels";

    } // namespace LoaderStrings

    //-------------------------------------------------------------------------------------------//
    EACEControlType TagToType(const AZStd::string_view tag)
    {
        // maybe a simple map would be better.
        if (tag == Audio::ATLXmlTags::ATLTriggerTag)
        {
            return eACET_TRIGGER;
        }
        else if (tag == Audio::ATLXmlTags::ATLSwitchTag)
        {
            return eACET_SWITCH;
        }
        else if (tag == Audio::ATLXmlTags::ATLSwitchStateTag)
        {
            return eACET_SWITCH_STATE;
        }
        else if (tag == Audio::ATLXmlTags::ATLRtpcTag)
        {
            return eACET_RTPC;
        }
        else if (tag == Audio::ATLXmlTags::ATLEnvironmentTag)
        {
            return eACET_ENVIRONMENT;
        }
        else if (tag == Audio::ATLXmlTags::ATLPreloadRequestTag)
        {
            return eACET_PRELOAD;
        }
        return eACET_NUM_TYPES;
    }

    //-------------------------------------------------------------------------------------------//
    CAudioControlsLoader::CAudioControlsLoader(CATLControlsModel* atlControlsModel, QStandardItemModel* layoutModel, IAudioSystemEditor* audioSystemImpl)
        : m_atlControlsModel(atlControlsModel)
        , m_layoutModel(layoutModel)
        , m_audioSystemImpl(audioSystemImpl)
    {}

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadAll()
    {
        LoadScopes();
        LoadControls();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadControls()
    {
        const CUndoSuspend suspendUndo;

        // Get the relative path (under asset root) where the controls live.
        const char* controlsPath = AZ::Interface<Audio::IAudioSystem>::Get()->GetControlsPath();

        // Get the full path up to asset root.
        AZ::IO::FixedMaxPath controlsFullPath = AZ::Utils::GetProjectPath();
        controlsFullPath /= controlsPath;

        // load the global controls
        LoadAllLibrariesInFolder(controlsFullPath.Native(), "");

        AZ::IO::FixedMaxPath searchPath = controlsFullPath / LoaderStrings::LevelsSubFolder;

        auto foundFiles = Audio::FindFilesInPath(searchPath.Native(), "*");

        for (const auto& file : foundFiles)
        {
            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(file.c_str()))
            {
                AZStd::string levelName{ file.Filename().Native() };
                LoadAllLibrariesInFolder(controlsFullPath.Native(), levelName);

                if (!m_atlControlsModel->ScopeExists(levelName))
                {
                    // If the scope doesn't exist it means it is not a real
                    // level in the project so it's flagged as LocalOnly
                    m_atlControlsModel->AddScope(levelName, true);
                }
            }
        }

        CreateDefaultControls();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadAllLibrariesInFolder(const AZStd::string_view folderPath, const AZStd::string_view level)
    {
        AZ::IO::FixedMaxPath searchPath{ folderPath };

        if (!level.empty())
        {
            searchPath /= LoaderStrings::LevelsSubFolder;
            searchPath /= level;
        }

        auto foundFiles = Audio::FindFilesInPath(searchPath.Native(), "*.xml");

        for (auto& file : foundFiles)
        {
            Audio::ScopedXmlLoader xmlLoader(file.Native());
            if (xmlLoader.HasError())
            {
                AZ_Warning("AudioControlsLoader", false, "Unable to load the xml file '%s'", file.c_str());
                continue;
            }

            auto xmlRootNode = xmlLoader.GetRootNode();
            if (xmlRootNode && azstricmp(xmlRootNode->name(), Audio::ATLXmlTags::RootNodeTag) == 0)
            {
                AZ::IO::PathView fileName = file.Filename();

                AZStd::to_lower(file.Native().begin(), file.Native().end());
                m_loadedFilenames.insert(file.c_str());

                if (auto nameAttr = xmlRootNode->first_attribute(Audio::ATLXmlTags::ATLNameAttribute, 0, false); nameAttr != nullptr)
                {
                    fileName = nameAttr->value();
                }
                else
                {
                    fileName = fileName.Stem();
                }

                LoadControlsLibrary(xmlRootNode, folderPath, level, fileName.Native());
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* CAudioControlsLoader::AddFolder(QStandardItem* parentItem, const QString& name)
    {
        if (parentItem && !name.isEmpty())
        {
            const int size = parentItem->rowCount();
            for (int i = 0; i < size; ++i)
            {
                QStandardItem* item = parentItem->child(i);
                if (item && (item->data(eDR_TYPE) == eIT_FOLDER) && (QString::compare(name, item->text(), Qt::CaseInsensitive) == 0))
                {
                    return item;
                }
            }

            QStandardItem* item = new QFolderItem(name);
            if (parentItem && item)
            {
                parentItem->appendRow(item);
                return item;
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* CAudioControlsLoader::AddUniqueFolderPath(QStandardItem* parentItem, const QString& path)
    {
        QStringList folderNames = path.split(QRegExp("(\\\\|\\/)"), Qt::SkipEmptyParts);
        const int size = folderNames.length();
        for (int i = 0; i < size; ++i)
        {
            if (!folderNames[i].isEmpty())
            {
                QStandardItem* childItem = AddFolder(parentItem, folderNames[i]);
                if (childItem)
                {
                    parentItem = childItem;
                }
            }
        }
        return parentItem;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadControlsLibrary(
        const AZ::rapidxml::xml_node<char>* rootNode,
        [[maybe_unused]] const AZStd::string_view filePath,
        const AZStd::string_view level,
        const AZStd::string_view fileName)
    {
        QStandardItem* rootFolderItem = AddUniqueFolderPath(m_layoutModel->invisibleRootItem(), QString(fileName.data()));
        if (rootFolderItem && rootNode)
        {
            auto controlTypeNode = rootNode->first_node();  // e.g. "AudioTriggers", "AudioRtpcs", etc
            while (controlTypeNode)
            {
                auto controlNode = controlTypeNode->first_node();   // e.g. "ATLTrigger", "ATLRtpc", etc
                while (controlNode)
                {
                    LoadControl(controlNode, rootFolderItem, level);
                    controlNode = controlNode->next_sibling();
                }
                controlTypeNode = controlTypeNode->next_sibling();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CAudioControlsLoader::LoadControl(AZ::rapidxml::xml_node<char>* node, QStandardItem* folderItem, const AZStd::string_view scope)
    {
        CATLControl* control = nullptr;

        AZStd::string controlPath;
        if (auto controlPathAttr = node->first_attribute("path", 0, false);
            controlPathAttr != nullptr)
        {
            controlPath = controlPathAttr->value();
        }

        QStandardItem* parentItem = AddUniqueFolderPath(folderItem, QString(controlPath.c_str()));
        if (parentItem)
        {
            AZStd::string name;
            if (auto nameAttr = node->first_attribute(Audio::ATLXmlTags::ATLNameAttribute, 0, false);
                nameAttr != nullptr)
            {
                name = nameAttr->value();
            }

            const EACEControlType controlType = TagToType(node->name());

            control = m_atlControlsModel->CreateControl(name, controlType);
            if (control)
            {
                QStandardItem* item = new QAudioControlItem(QString(control->GetName().c_str()), control);
                if (item)
                {
                    parentItem->appendRow(item);
                }

                switch (controlType)
                {
                    case eACET_SWITCH:
                    {
                        auto switchStateNode = node->first_node();
                        while (switchStateNode)
                        {
                            CATLControl* stateControl = LoadControl(switchStateNode, item, scope);
                            if (stateControl)
                            {
                                stateControl->SetParent(control);
                                control->AddChild(stateControl);
                            }

                            switchStateNode = switchStateNode->next_sibling();
                        }
                        break;
                    }
                    case eACET_PRELOAD:
                    {
                        LoadPreloadConnections(node, control);
                        break;
                    }
                    default:
                    {
                        LoadConnections(node, control);
                        break;
                    }
                }

                control->SetScope(scope);
            }
        }

        return control;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadScopes()
    {
        AZ::IO::FixedMaxPath levelsFolderPath = AZ::Utils::GetProjectPath();
        levelsFolderPath /= "Levels";
        LoadScopesImpl(levelsFolderPath.Native());
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadScopesImpl(const AZStd::string_view levelsFolder)
    {
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FixedMaxPath searchPath{ levelsFolder };

        auto foundFiles = Audio::FindFilesInPath(searchPath.Native(), "*");
        for (auto& file : foundFiles)
        {
            AZ::IO::PathView filePath{ file };
            AZ::IO::PathView fileName = filePath.Filename();
            if (fileIO->IsDirectory(filePath.Native().data()))
            {
                LoadScopesImpl((searchPath / fileName).Native());
            }
            else
            {
                AZ::IO::PathView fileExt = filePath.Extension();
                if (fileExt == ".ly" || fileExt == ".cry" || fileExt == ".prefab")
                {
                    AZ::IO::PathView fileStem = filePath.Stem();
                    // May need to verify that .prefabs are the actual "level" prefab
                    // i.e. that it matches levels/<levelname>/<levelname>.prefab
                    m_atlControlsModel->AddScope(fileStem.Native());
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    const FilepathSet& CAudioControlsLoader::GetLoadedFilenamesList()
    {
        return m_loadedFilenames;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::CreateDefaultControls()
    {
        // Load default controls if the don't exist.
        // These controls need to always exist in your project
        using namespace Audio;

        QStandardItem* folderItem = AddFolder(m_layoutModel->invisibleRootItem(), "default_controls");
        if (folderItem)
        {
            if (!m_atlControlsModel->FindControl(ATLInternalControlNames::GetFocusName, eACET_TRIGGER, ""))
            {
                AddControl(m_atlControlsModel->CreateControl(ATLInternalControlNames::GetFocusName, eACET_TRIGGER), folderItem);
            }

            if (!m_atlControlsModel->FindControl(ATLInternalControlNames::LoseFocusName, eACET_TRIGGER, ""))
            {
                AddControl(m_atlControlsModel->CreateControl(ATLInternalControlNames::LoseFocusName, eACET_TRIGGER), folderItem);
            }

            if (!m_atlControlsModel->FindControl(ATLInternalControlNames::MuteAllName, eACET_TRIGGER, ""))
            {
                AddControl(m_atlControlsModel->CreateControl(ATLInternalControlNames::MuteAllName, eACET_TRIGGER), folderItem);
            }

            if (!m_atlControlsModel->FindControl(ATLInternalControlNames::UnmuteAllName, eACET_TRIGGER, ""))
            {
                AddControl(m_atlControlsModel->CreateControl(ATLInternalControlNames::UnmuteAllName, eACET_TRIGGER), folderItem);
            }

            if (!m_atlControlsModel->FindControl(ATLInternalControlNames::DoNothingName, eACET_TRIGGER, ""))
            {
                AddControl(m_atlControlsModel->CreateControl(ATLInternalControlNames::DoNothingName, eACET_TRIGGER), folderItem);
            }

            if (!m_atlControlsModel->FindControl(ATLInternalControlNames::ObjectSpeedName, eACET_RTPC, ""))
            {
                AddControl(m_atlControlsModel->CreateControl(ATLInternalControlNames::ObjectSpeedName, eACET_RTPC), folderItem);
            }

            QStandardItem* switchItem = nullptr;
            CATLControl* control = m_atlControlsModel->FindControl(ATLInternalControlNames::ObstructionOcclusionCalcName, eACET_SWITCH, "");
            if (control)
            {
                QModelIndexList indexes = m_layoutModel->match(m_layoutModel->index(0, 0, QModelIndex()), eDR_ID, control->GetId(), 1, Qt::MatchRecursive);
                if (!indexes.empty())
                {
                    switchItem = m_layoutModel->itemFromIndex(indexes.at(0));
                }
            }
            else
            {
                control = m_atlControlsModel->CreateControl(ATLInternalControlNames::ObstructionOcclusionCalcName, eACET_SWITCH);
                switchItem = AddControl(control, folderItem);
            }

            if (switchItem)
            {
                CATLControl* childControl = nullptr;
                if (!m_atlControlsModel->FindControl(ATLInternalControlNames::OOCIgnoreStateName, eACET_SWITCH_STATE, "", control))
                {
                    childControl = CreateInternalSwitchState(control, ATLInternalControlNames::ObstructionOcclusionCalcName, ATLInternalControlNames::OOCIgnoreStateName);
                    AddControl(childControl, switchItem);
                }

                if (!m_atlControlsModel->FindControl(ATLInternalControlNames::OOCSingleRayStateName, eACET_SWITCH_STATE, "", control))
                {
                    childControl = CreateInternalSwitchState(control, ATLInternalControlNames::ObstructionOcclusionCalcName, ATLInternalControlNames::OOCSingleRayStateName);
                    AddControl(childControl, switchItem);
                }

                if (!m_atlControlsModel->FindControl(ATLInternalControlNames::OOCMultiRayStateName, eACET_SWITCH_STATE, "", control))
                {
                    childControl = CreateInternalSwitchState(control, ATLInternalControlNames::ObstructionOcclusionCalcName, ATLInternalControlNames::OOCMultiRayStateName);
                    AddControl(childControl, switchItem);
                }
            }

            switchItem = nullptr;
            control = m_atlControlsModel->FindControl(ATLInternalControlNames::ObjectVelocityTrackingName, eACET_SWITCH, "");
            if (control)
            {
                QModelIndexList indexes = m_layoutModel->match(m_layoutModel->index(0, 0, QModelIndex()), eDR_ID, control->GetId(), 1, Qt::MatchRecursive);
                if (!indexes.empty())
                {
                    switchItem = m_layoutModel->itemFromIndex(indexes.at(0));
                }
            }
            else
            {
                control = m_atlControlsModel->CreateControl(ATLInternalControlNames::ObjectVelocityTrackingName, eACET_SWITCH);
                switchItem = AddControl(control, folderItem);
            }

            if (switchItem)
            {
                CATLControl* childControl = nullptr;
                if (!m_atlControlsModel->FindControl(ATLInternalControlNames::OVTOnStateName, eACET_SWITCH_STATE, "", control))
                {
                    childControl = CreateInternalSwitchState(control, ATLInternalControlNames::ObjectVelocityTrackingName, ATLInternalControlNames::OVTOnStateName);
                    AddControl(childControl, switchItem);
                }

                if (!m_atlControlsModel->FindControl(ATLInternalControlNames::OVTOffStateName, eACET_SWITCH_STATE, "", control))
                {
                    childControl = CreateInternalSwitchState(control, ATLInternalControlNames::ObjectVelocityTrackingName, ATLInternalControlNames::OVTOffStateName);
                    AddControl(childControl, switchItem);
                }

                if (!folderItem->hasChildren())
                {
                    m_layoutModel->removeRow(folderItem->row(), m_layoutModel->indexFromItem(folderItem->parent()));
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadConnections(AZ::rapidxml::xml_node<char>* rootNode, CATLControl* control)
    {
        if (control && rootNode && m_audioSystemImpl)
        {
            auto childNode = rootNode->first_node();
            while (childNode)
            {
                TConnectionPtr connection = m_audioSystemImpl->CreateConnectionFromXMLNode(childNode, control->GetType());
                if (connection)
                {
                    control->AddConnection(connection);
                }

                control->m_connectionNodes.emplace_back(childNode, connection != nullptr);

                childNode = childNode->next_sibling();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadPreloadConnections(AZ::rapidxml::xml_node<char>* node, CATLControl* control)
    {
        if (!control || !node || !m_audioSystemImpl)
        {
            return;
        }

        AZStd::string type;
        if (auto typeAttr = node->first_attribute(Audio::ATLXmlTags::ATLTypeAttribute, 0, false);
            typeAttr != nullptr)
        {
            type = typeAttr->value();
        }

        control->SetAutoLoad(type == Audio::ATLXmlTags::ATLDataLoadType);

        auto platformGroupNode = node->first_node(Audio::ATLXmlTags::ATLPlatformsTag, 0, false);
        if (platformGroupNode)
        {
            // Legacy preload parsing...
            // Don't parse the platform groups xml chunk anymore.

            // Read the connection information for all connected preloads...
            auto configGroupNode = node->first_node(Audio::ATLXmlTags::ATLConfigGroupTag, 0, false);
            while (configGroupNode)
            {
                auto connectionNode = configGroupNode->first_node();
                while (connectionNode)
                {
                    TConnectionPtr connection = m_audioSystemImpl->CreateConnectionFromXMLNode(connectionNode, control->GetType());
                    if (connection)
                    {
                        control->AddConnection(connection);
                    }
                    control->m_connectionNodes.emplace_back(connectionNode, connection != nullptr);
                    connectionNode = connectionNode->next_sibling();
                }
                configGroupNode = configGroupNode->next_sibling();
            }
        }
        else
        {
            // New format preload parsing...
            auto connectionNode = node->first_node();
            while (connectionNode)
            {
                TConnectionPtr connection = m_audioSystemImpl->CreateConnectionFromXMLNode(connectionNode, control->GetType());
                if (connection)
                {
                    control->AddConnection(connection);
                }
                control->m_connectionNodes.emplace_back(connectionNode, connection != nullptr);
                connectionNode = connectionNode->next_sibling();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* CAudioControlsLoader::AddControl(CATLControl* control, QStandardItem* folderItem)
    {
        QStandardItem* item = new QAudioControlItem(QString(control->GetName().c_str()), control);
        if (item)
        {
            item->setData(true, eDR_MODIFIED);
            folderItem->appendRow(item);
        }
        return item;
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CAudioControlsLoader::CreateInternalSwitchState(CATLControl* parentControl, const AZStd::string& switchName, const AZStd::string& stateName)
    {
        CATLControl* childControl = m_atlControlsModel->CreateControl(stateName, eACET_SWITCH_STATE, parentControl);

        XmlAllocator& xmlAlloc(AudioControls::s_xmlAllocator);
        AZ::rapidxml::xml_node<char>* requestNode =
            xmlAlloc.allocate_node(AZ::rapidxml::node_element, xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLSwitchRequestTag));

        AZ::rapidxml::xml_attribute<char>* switchNameAttr = xmlAlloc.allocate_attribute(
            xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLNameAttribute), xmlAlloc.allocate_string(switchName.c_str()));

        requestNode->append_attribute(switchNameAttr);

        AZ::rapidxml::xml_node<char>* valueNode =
            xmlAlloc.allocate_node(AZ::rapidxml::node_element, xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLValueTag));

        AZ::rapidxml::xml_attribute<char>* stateNameAttr = xmlAlloc.allocate_attribute(
            xmlAlloc.allocate_string(Audio::ATLXmlTags::ATLNameAttribute), xmlAlloc.allocate_string(stateName.c_str()));

        valueNode->append_attribute(stateNameAttr);

        requestNode->append_node(valueNode);

        childControl->m_connectionNodes.emplace_back(requestNode, false);
        return childControl;
    }

} // namespace AudioControls
