/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ACETypes.h>
#include <AzCore/std/string/string_view.h>
#include <AudioSystemControl_wwise.h>

#include <AzCore/XML/rapidxml.h>

namespace AudioControls
{
    class CAudioSystemEditor_wwise;

    //-------------------------------------------------------------------------------------------//
    class CAudioWwiseLoader
    {
    public:
        CAudioWwiseLoader() = default;
        void Load(CAudioSystemEditor_wwise* audioSystemImpl);
        const AZStd::string& GetLocalizationFolder() const;

    private:
        void LoadSoundBanks(const AZStd::string_view rootFolder, const AZStd::string_view subPath, bool isLocalized);
        void LoadControlsInFolder(const AZStd::string_view folderPath);
        void LoadControl(const AZ::rapidxml::xml_node<char>* xmlNode);
        void ExtractControlsFromXML(const AZ::rapidxml::xml_node<char>* xmlNode, EWwiseControlTypes type, const AZStd::string_view controlTag, const AZStd::string_view controlNameAttribute);

    private:
        AZStd::string m_localizationFolder;
        CAudioSystemEditor_wwise* m_audioSystemImpl = nullptr;
    };
} // namespace AudioControls
