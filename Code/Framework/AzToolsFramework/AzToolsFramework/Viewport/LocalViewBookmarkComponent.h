/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    //! @class LocalViewBookmarkComponent.
    //! @brief Component that stores the name of the local View bookmark file associated to the prefab.
    class LocalViewBookmarkComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:

        AZ_EDITOR_COMPONENT(LocalViewBookmarkComponent, "{28E5C3B7-3732-4EFB-985E-3F578A83A357}", EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        const AZStd::string& GetLocalBookmarksFileName() const;
        void SetLocalBookmarksFileName(AZStd::string localBookmarksFileName);

    private:
        //! name of the local View bookmark file associated to the prefab. located in project/user/SettingsRegistry/ViewBookmarks.
        AZStd::string m_localBookmarksFileName;
    };
} // namespace AzToolsFramework
