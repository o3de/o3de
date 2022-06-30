/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <GemCatalog/GemCatalogScreen.h>
#endif

namespace O3DE::ProjectManager
{
    //! A wrapper for a GemCatalogScreen that shows what gems are active in a project 
    class ProjectGemCatalogScreen
        : public GemCatalogScreen
    {
    public:
        explicit ProjectGemCatalogScreen(QWidget* parent = nullptr);
        ~ProjectGemCatalogScreen() = default;

        ProjectManagerScreen GetScreenEnum() override;
        bool IsTab() override;

        enum class ConfiguredGemsResult 
        {
            Failed = 0,
            Success,
            Cancel
        };
        ConfiguredGemsResult ConfigureGemsForProject(const QString& projectPath);
    };

} // namespace O3DE::ProjectManager
