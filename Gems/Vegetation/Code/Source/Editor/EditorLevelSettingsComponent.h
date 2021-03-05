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
#pragma once

#include <LmbrCentral/Component/EditorWrappedComponentBase.h>
#include <Components/LevelSettingsComponent.h>

namespace Vegetation
{
    class EditorLevelSettingsComponent
        : public LmbrCentral::EditorWrappedComponentBase<LevelSettingsComponent, LevelSettingsConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<LevelSettingsComponent, LevelSettingsConfig>;
        AZ_EDITOR_COMPONENT(EditorLevelSettingsComponent, "{F2EF4820-88D1-41C3-BFB3-BAC3C7B494E3}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);
        
        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation System Settings";
        static constexpr const char* const s_componentDescription = "The vegetation system settings for this level/map.";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-system-settings";

    private:
        AZ::u32 ConfigurationChanged() override;

        bool m_useEditorMaxInstanceProcessTimeMicroseconds = false;
        int m_editorMaxInstanceProcessTimeMicroseconds = 33000;
    };

} // namespace Vegetation
