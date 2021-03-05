/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
