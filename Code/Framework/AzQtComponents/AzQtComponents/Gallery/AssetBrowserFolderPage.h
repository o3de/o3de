/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui
{
    class AssetBrowserFolderPage;
}

class AssetBrowserFolderPage : public QWidget
{
    Q_OBJECT //AUTOMOC

public:
    explicit AssetBrowserFolderPage(QWidget* parent = nullptr);
    ~AssetBrowserFolderPage() override;

private:
    QScopedPointer<Ui::AssetBrowserFolderPage> ui;
};
