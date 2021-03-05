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
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QMainWindow>
#endif


namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API DockMainWindow
        : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit DockMainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
        QMenu* createPopupMenu() override;

        /// Set if this main window is owned by a fancy docking instance
        void SetFancyDockingOwner(QWidget* instance);
        /// Return whether or not this main window is configured with fancy docking
        bool HasFancyDocking();
    };
} // namespace AzQtComponents
