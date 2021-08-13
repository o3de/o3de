/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/StyledDockWidget.h>
#endif

namespace EMStudio
{
    class EMStudioPlugin;
    
    class RemovePluginOnCloseDockWidget
        : public AzQtComponents::StyledDockWidget
    {
        Q_OBJECT

    public:
        RemovePluginOnCloseDockWidget(QWidget* parent, const QString& name, EMStudio::EMStudioPlugin* plugin);

    protected:
        void closeEvent(QCloseEvent* event) override;

    private:
        EMStudio::EMStudioPlugin* m_plugin;
    };
}
