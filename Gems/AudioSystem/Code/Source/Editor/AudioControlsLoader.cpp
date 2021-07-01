/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControlsLoader.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <ACEEnums.h>
#include <ATLCommon.h>
#include <ATLControlsModel.h>
#include <IAudioSystem.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <QAudioControlTreeWidget.h>

#include <CryFile.h>
#include <CryPath.h>
#include <IEditor.h>
#include <ISystem.h>
#include <StringUtils.h>
#include <Util/PathUtil.h>

#include <QStandardItem>

using namespace PathUtil;

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    namespace LoaderStrings
    {
        static constexpr const char* LevelsSubFolder = "levels";
        static constexpr const char* PathAttribute = "path";

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

        // Get the partial path (relative under asset root) where the controls live.
        const char* controlsPath = nullptr;
        Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);

        // Get the full path up to asset root.
        AZStd::string controlsFullPath(Path::GetEditingGameDataFolder());
        AZ::StringFunc::Path::Join(controlsFullPath.c_str(), controlsPath, controlsFullPath);

        // load the global controls
        LoadAllLibrariesInFolder(controlsFullPath, "");

        // load the level specific controls
        auto cryPak = gEnv->pCryPak;

        AZStd::string searchMask;
        AZ::StringFunc::Path::Join(controlsFullPath.c_str(), LoaderStrings::LevelsSubFolder, searchMask);
        AZ::StringFunc::Path::Join(searchMask.c_str(), "*", searchMask, true, false);
        AZ::IO::ArchiveFileIterator handle = cryPak->FindFirst(searchMask.c_str());
        if (handle)
        {
            do
            {
                if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    AZStd::string_view name = handle.m_filename;
                    if (name != "." && name != "..")
                    {
                        LoadAllLibrariesInFolder(controlsFullPath, name);
                        if (!m_atlControlsModel->ScopeExists(name))
                        {
                            // if the control doesn't exist it
                            // means it is not a real level in the
                            // project so it is flagged as LocalOnly
                            m_atlControlsModel->AddScope(name, true);
                        }
                    }
                }
            }
            while (handle = cryPak->FindNext(handle));
            cryPak->FindClose(handle);
        }
        CreateDefaultControls();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadAllLibrariesInFolder(const AZStd::string_view folderPath, const AZStd::string_view level)
    {
        AZStd::string path(folderPath);
        if (path.back() != AZ_CORRECT_FILESYSTEM_SEPARATOR)
        {
            path.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        }

        if (!level.empty())
        {
            path.append(LoaderStrings::LevelsSubFolder);
            path.append(GetSlash());
            path.append(level);
            path.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        }

        AZStd::string searchPath = path + "*.xml";
        auto cryPak = gEnv->pCryPak;
        AZ::IO::ArchiveFileIterator handle = cryPak->FindFirst(searchPath.c_str());
        if (handle)
        {
            do
            {
                AZStd::string filename = path + AZStd::string{ static_cast<AZStd::string_view>(handle.m_filename) };
                AZ::StringFunc::Path::Normalize(filename);
                XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename.c_str());
                if (root)
                {
                    AZStd::string tag = root->getTag();
                    if (tag == Audio::ATLXmlTags::RootNodeTag)
                    {
                        AZStd::to_lower(filename.begin(), filename.end());
                        m_loadedFilenames.insert(filename.c_str());
                        AZStd::string file = static_cast<AZStd::string_view>(handle.m_filename);
                        if (root->haveAttr(Audio::ATLXmlTags::ATLNameAttribute))
                        {
                            file = root->getAttr(Audio::ATLXmlTags::ATLNameAttribute);
                        }
                        AZ::StringFunc::Path::StripExtension(file);
                        LoadControlsLibrary(root, folderPath, level, file);
                    }
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "(Audio Controls Editor) Failed parsing ATL Library '%s'", filename.c_str());
                }
            } while (handle = cryPak->FindNext(handle));

            cryPak->FindClose(handle);
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
    void CAudioControlsLoader::LoadControlsLibrary(XmlNodeRef rootNode, [[maybe_unused]] const AZStd::string_view filePath, const AZStd::string_view level, const AZStd::string_view fileName)
    {
        QStandardItem* rootFolderItem = AddUniqueFolderPath(m_layoutModel->invisibleRootItem(), QString(fileName.data()));
        if (rootFolderItem && rootNode)
        {
            const int numControlTypes = rootNode->getChildCount();
            for (int i = 0; i < numControlTypes; ++i)
            {
                XmlNodeRef node = rootNode->getChild(i);
                const int numControls = node->getChildCount();
                for (int j = 0; j < numControls; ++j)
                {
                    LoadControl(node->getChild(j), rootFolderItem, level);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CAudioControlsLoader::LoadControl(XmlNodeRef node, QStandardItem* folderItem, const AZStd::string_view scope)
    {
        CATLControl* control = nullptr;
        if (node)
        {
            QStandardItem* parentItem = AddUniqueFolderPath(folderItem, QString(node->getAttr(LoaderStrings::PathAttribute)));
            if (parentItem)
            {
                const AZStd::string name = node->getAttr(Audio::ATLXmlTags::ATLNameAttribute);
                const EACEControlType controlType = TagToType(node->getTag());

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
                            const int numStates = node->getChildCount();
                            for (int i = 0; i < numStates; ++i)
                            {
                                CATLControl* stateControl = LoadControl(node->getChild(i), item, scope);
                                if (stateControl)
                                {
                                    stateControl->SetParent(control);
                                    control->AddChild(stateControl);
                                }
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
        }

        return control;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadScopes()
    {
        AZStd::string levelsFolderPath;
        AZ::StringFunc::Path::Join(Path::GetEditingGameDataFolder().c_str(), LoaderStrings::LevelsSubFolder, levelsFolderPath);
        LoadScopesImpl(levelsFolderPath);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadScopesImpl(const AZStd::string_view levelsFolder)
    {
        AZStd::string search;
        AZ::StringFunc::Path::Join(levelsFolder.data(), "*", search, true, false);
        auto cryPak = gEnv->pCryPak;
        AZ::IO::ArchiveFileIterator handle = cryPak->FindFirst(search.c_str());
        if (handle)
        {
            do
            {
                AZStd::string name = static_cast<AZStd::string_view>(handle.m_filename);
                if (name != "." && name != ".." && !name.empty())
                {
                    if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                    {
                        AZ::StringFunc::Path::Join(levelsFolder.data(), name.c_str(), search);
                        LoadScopesImpl(search);
                    }
                    else
                    {
                        AZStd::string extension;
                        AZ::StringFunc::Path::GetExtension(name.c_str(), extension, false);
                        if (extension.compare("cry") == 0 || extension.compare("ly") == 0)
                        {
                            AZ::StringFunc::Path::StripExtension(name);
                            m_atlControlsModel->AddScope(name);
                        }
                    }
                }
            }
            while (handle = cryPak->FindNext(handle));
            cryPak->FindClose(handle);
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
    void CAudioControlsLoader::LoadConnections(XmlNodeRef rootNode, CATLControl* control)
    {
        if (!rootNode || !control)
        {
            return;
        }

        const int numChildren = rootNode->getChildCount();
        for (int i = 0; i < numChildren; ++i)
        {
            XmlNodeRef node = rootNode->getChild(i);
            const AZStd::string tag = node->getTag();
            if (m_audioSystemImpl)
            {
                TConnectionPtr connection = m_audioSystemImpl->CreateConnectionFromXMLNode(node, control->GetType());
                if (connection)
                {
                    control->AddConnection(connection);
                }
                control->m_connectionNodes.push_back(SRawConnectionData(node, connection != nullptr));
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadPreloadConnections(XmlNodeRef node, CATLControl* control)
    {
        if (!node || !control)
        {
            return;
        }

        AZStd::string type = node->getAttr(Audio::ATLXmlTags::ATLTypeAttribute);
        if (type.compare(Audio::ATLXmlTags::ATLDataLoadType) == 0)
        {
            control->SetAutoLoad(true);
        }
        else
        {
            control->SetAutoLoad(false);
        }

        // Legacy Preload XML parsing...
        // Read all the platform definitions for this control
        XmlNodeRef platformsGroupNode = node->findChild(Audio::ATLXmlTags::ATLPlatformsTag);
        if (platformsGroupNode)
        {
            // Don't parse the platform groups xml chunk anymore.

            // Read the connection information for all connected preloads...
            const int numChildren = node->getChildCount();
            for (int i = 0; i < numChildren; ++i)
            {
                XmlNodeRef groupNode = node->getChild(i);
                const AZStd::string tag = groupNode->getTag();
                if (tag.compare(Audio::ATLXmlTags::ATLConfigGroupTag) != 0)
                {
                    continue;
                }

                const AZStd::string groupName = groupNode->getAttr(Audio::ATLXmlTags::ATLNameAttribute);
                const int numConnections = groupNode->getChildCount();
                for (int j = 0; j < numConnections; ++j)
                {
                    XmlNodeRef connectionNode = groupNode->getChild(j);
                    if (connectionNode && m_audioSystemImpl)
                    {
                        TConnectionPtr connection = m_audioSystemImpl->CreateConnectionFromXMLNode(connectionNode, control->GetType());
                        if (connection)
                        {
                            control->AddConnection(connection);
                        }
                        control->m_connectionNodes.push_back(SRawConnectionData(connectionNode, connection != nullptr));
                    }
                }
            }
        }
        else
        {
            // New Preload XML parsing...
            const int numChildren = node->getChildCount();
            for (int i = 0; i < numChildren; ++i)
            {
                XmlNodeRef connectionNode = node->getChild(i);
                if (connectionNode && m_audioSystemImpl)
                {
                    TConnectionPtr connection = m_audioSystemImpl->CreateConnectionFromXMLNode(connectionNode, control->GetType());
                    if (connection)
                    {
                        control->AddConnection(connection);
                    }

                    control->m_connectionNodes.push_back(SRawConnectionData(connectionNode, connection != nullptr));
                }
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

        XmlNodeRef requestNode = GetISystem()->CreateXmlNode(Audio::ATLXmlTags::ATLSwitchRequestTag);
        requestNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, switchName.c_str());
        XmlNodeRef valueNode = requestNode->createNode(Audio::ATLXmlTags::ATLValueTag);
        valueNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, stateName.c_str());
        requestNode->addChild(valueNode);

        childControl->m_connectionNodes.push_back(SRawConnectionData(requestNode, false));
        return childControl;
    }

} // namespace AudioControls
