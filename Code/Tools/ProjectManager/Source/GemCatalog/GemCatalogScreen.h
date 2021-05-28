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

#if !defined(Q_MOC_RUN)
#include <ScreenWidget.h>
#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemModel.h>
#endif

namespace O3DE::ProjectManager
{
    class GemCatalogScreen
        : public ScreenWidget
    {
    public:
        explicit GemCatalogScreen(QWidget* parent = nullptr);
        ~GemCatalogScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

    private:
        QVector<GemInfo> GenerateTestData();

        GemListView* m_gemListView = nullptr;
        GemInspector* m_gemInspector = nullptr;
        GemModel* m_gemModel = nullptr;
    };
} // namespace O3DE::ProjectManager
