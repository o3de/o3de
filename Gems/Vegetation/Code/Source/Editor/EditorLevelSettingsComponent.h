/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";

    private:
        AZ::u32 ConfigurationChanged() override;

        bool m_useEditorMaxInstanceProcessTimeMicroseconds = false;
        int m_editorMaxInstanceProcessTimeMicroseconds = 33000;
    };

} // namespace Vegetation
