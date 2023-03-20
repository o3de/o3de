/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <QPointer>
#include <AzCore/IO/Path/Path.h>
#include <ScreenDefs.h>
#endif

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(DownloadController);

    class ProjectManagerWindow
        : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit ProjectManagerWindow(QWidget* parent, const AZ::IO::PathView& projectPath,
            ProjectManagerScreen startScreen = ProjectManagerScreen::Projects);

    private:
        QPointer<DownloadController> m_downloadController;
    };

} // namespace O3DE::ProjectManager
