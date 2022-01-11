/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/limits.h>
#include <AzCore/Interface/Interface.h>

namespace AzToolsFramework
{
    class EditorSettingsBlock;

    using SubCategoryMap = AZStd::map<AZStd::string, EditorSettingsBlock*>;
    using CategoryMap = AZStd::map<AZStd::string, SubCategoryMap>;

    /*!
     * EditorSettingsInterface
     */

    class EditorSettingsInterface
    {
    public:
        AZ_RTTI(EditorSettingsInterface, "{E7479325-6DA2-4E62-938C-EE4F4727F94B}");

        virtual void OpenEditorSettingsDialog() = 0;
        virtual CategoryMap* GetSettingsBlocks() = 0;
    };
} // namespace AzToolsFramework::Editor
