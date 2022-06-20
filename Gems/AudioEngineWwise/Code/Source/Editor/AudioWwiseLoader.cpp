/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioWwiseLoader.h>

#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <AudioSystemEditor_wwise.h>
#include <AudioFileUtils.h>
#include <Config_wwise.h>


namespace AudioControls
{
    namespace WwiseStrings
    {
        // Wwise Project Folders
        static constexpr const char GameParametersFolder[] = "Game Parameters";
        static constexpr const char GameStatesFolder[] = "States";
        static constexpr const char SwitchesFolder[] = "Switches";
        static constexpr const char EventsFolder[] = "Events";
        static constexpr const char EnvironmentsFolder[] = "Master-Mixer Hierarchy";

        // Wwise Xml Tags
        static constexpr const char GameParameterTag[] = "GameParameter";
        static constexpr const char EventTag[] = "Event";
        static constexpr const char AuxBusTag[] = "AuxBus";
        static constexpr const char SwitchGroupTag[] = "SwitchGroup";
        static constexpr const char StateGroupTag[] = "StateGroup";
        static constexpr const char ChildrenListTag[] = "ChildrenList";
        static constexpr const char NameAttribute[] = "Name";

    } // namespace WwiseStrings

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::Load(CAudioSystemEditor_wwise* audioSystemImpl)
    {
        m_audioSystemImpl = audioSystemImpl;
        const AZ::IO::FixedMaxPath wwiseProjectFullPath{ m_audioSystemImpl->GetDataPath() };
        LoadControlsInFolder(AZ::IO::FixedMaxPath{ wwiseProjectFullPath / WwiseStrings::GameParametersFolder }.Native());
        LoadControlsInFolder(AZ::IO::FixedMaxPath{ wwiseProjectFullPath / WwiseStrings::GameStatesFolder }.Native());
        LoadControlsInFolder(AZ::IO::FixedMaxPath{ wwiseProjectFullPath / WwiseStrings::SwitchesFolder }.Native());
        LoadControlsInFolder(AZ::IO::FixedMaxPath{ wwiseProjectFullPath / WwiseStrings::EventsFolder }.Native());
        LoadControlsInFolder(AZ::IO::FixedMaxPath{ wwiseProjectFullPath / WwiseStrings::EnvironmentsFolder }.Native());
        LoadSoundBanks(Audio::Wwise::GetBanksRootPath(), "", false);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadSoundBanks(const AZStd::string_view rootFolder, const AZStd::string_view subPath, bool isLocalized)
    {
        AZ::IO::FixedMaxPath searchPath(rootFolder);
        searchPath /= subPath;
        auto foundFiles = Audio::FindFilesInPath(searchPath.Native(), "*");
        bool isLocalizedLoaded = isLocalized;

        for (const auto& filePath : foundFiles)
        {
            AZ_Assert(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()), "FindFiles found file '%s' but FileIO says it doesn't exist!", filePath.c_str());
            AZ::IO::PathView fileName = filePath.Filename();

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(filePath.c_str()))
            {
                if (fileName != Audio::Wwise::ExternalSourcesPath && !isLocalizedLoaded)
                {
                    // each sub-folder represents a different language,
                    // we load only one as all of them should have the
                    // same content (in the future we want to have a
                    // consistency report to highlight if this is not the case)
                    m_localizationFolder.assign(fileName.Native().data(), fileName.Native().size());
                    LoadSoundBanks(searchPath.Native(), m_localizationFolder, true);
                    isLocalizedLoaded = true;
                }
            }
            else if (fileName.Extension() == Audio::Wwise::BankExtension && fileName != Audio::Wwise::InitBank)
            {
                m_audioSystemImpl->CreateControl(
                    SControlDef(AZStd::string{ fileName.Native() }, eWCT_WWISE_SOUND_BANK, isLocalized, nullptr, subPath));
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadControlsInFolder(const AZStd::string_view folderPath)
    {
        auto foundFiles = Audio::FindFilesInPath(folderPath, "*");

        for (const auto& filePath : foundFiles)
        {
            AZ_Assert(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()), "FindFiles found file '%s' but FileIO says it doesn't exist!", filePath.c_str());

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(filePath.c_str()))
            {
                LoadControlsInFolder(filePath.Native());
            }
            else
            {
                // Open the file, read into an xmlDoc, and call LoadControls with the root xml node...
                AZ_TracePrintf("AudioWwiseLoader", "Loading Xml from '%s'", filePath.c_str());

                Audio::ScopedXmlLoader xmlFileLoader(filePath.Native());
                if (!xmlFileLoader.HasError())
                {
                    LoadControl(xmlFileLoader.GetRootNode());
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::ExtractControlsFromXML(const AZ::rapidxml::xml_node<char>* xmlNode, EWwiseControlTypes type, const AZStd::string_view controlTag, const AZStd::string_view controlNameAttribute)
    {
        AZStd::string_view xmlTag(xmlNode->name());
        if (xmlTag == controlTag)
        {
            if (auto nameAttr = xmlNode->first_attribute(controlNameAttribute.data()))
            {
                m_audioSystemImpl->CreateControl(SControlDef(nameAttr->value(), type));
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadControl(const AZ::rapidxml::xml_node<char>* xmlNode)
    {
        if (!xmlNode)
        {
            return;
        }

        ExtractControlsFromXML(xmlNode, eWCT_WWISE_RTPC, WwiseStrings::GameParameterTag, WwiseStrings::NameAttribute);
        ExtractControlsFromXML(xmlNode, eWCT_WWISE_EVENT, WwiseStrings::EventTag, WwiseStrings::NameAttribute);
        ExtractControlsFromXML(xmlNode, eWCT_WWISE_AUX_BUS, WwiseStrings::AuxBusTag, WwiseStrings::NameAttribute);

        AZStd::string_view xmlTag(xmlNode->name());
        bool isSwitchTag = (xmlTag == WwiseStrings::SwitchGroupTag);
        bool isStateTag = (xmlTag == WwiseStrings::StateGroupTag);

        if (isSwitchTag || isStateTag)
        {
            if (auto nameAttr = xmlNode->first_attribute(WwiseStrings::NameAttribute))
            {
                const AZStd::string parentName(nameAttr->value());
                IAudioSystemControl* group = m_audioSystemImpl->GetControlByName(parentName);
                if (!group)
                {
                    group = m_audioSystemImpl->CreateControl(SControlDef(parentName, isSwitchTag ? eWCT_WWISE_SWITCH_GROUP : eWCT_WWISE_GAME_STATE_GROUP));
                }

                auto childrenNode = xmlNode->first_node(WwiseStrings::ChildrenListTag);
                if (childrenNode)
                {
                    auto childNode = childrenNode->first_node();
                    while (childNode)
                    {
                        if (auto childNameAttr = childNode->first_attribute(WwiseStrings::NameAttribute))
                        {
                            m_audioSystemImpl->CreateControl(SControlDef(childNameAttr->value(), isSwitchTag ? eWCT_WWISE_SWITCH : eWCT_WWISE_GAME_STATE, false, group));
                        }
                        childNode = childNode->next_sibling();
                    }
                }
            }
        }

        auto childNode = xmlNode->first_node();
        while (childNode)
        {
            LoadControl(childNode);
            childNode = childNode->next_sibling();
        }
    }

    //-------------------------------------------------------------------------------------------//
    const AZStd::string& CAudioWwiseLoader::GetLocalizationFolder() const
    {
        return m_localizationFolder;
    }

} // namespace AudioControls
