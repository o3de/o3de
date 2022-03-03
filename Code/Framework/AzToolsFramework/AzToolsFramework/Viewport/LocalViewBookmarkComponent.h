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
    class LocalViewBookmarkComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        static constexpr const char* const ViewBookmarkComponentTypeId = "{28E5C3B7-3732-4EFB-985E-3F578A83A357}";

        AZ_COMPONENT(LocalViewBookmarkComponent, ViewBookmarkComponentTypeId, EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component overrides ...
        void Activate() override{};
        void Deactivate() override{};
        AZStd::string GetLocalBookmarksFileName() const;
        void SetLocalBookmarksFileName(const AZStd::string& localBookmarksFileName);

    private:
        AZStd::string m_localBookmarksFileName;
    };
} // namespace AzToolsFramework
