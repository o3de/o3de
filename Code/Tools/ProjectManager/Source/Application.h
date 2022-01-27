/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Application/Application.h>
#include <QCoreApplication>
#include <PythonBindings.h>
#include <ProjectManagerWindow.h>
#endif

namespace AZ
{
    class Entity;
}

namespace O3DE::ProjectManager
{
    class Application
        : public AzFramework::Application
    {
    public:
        using AzFramework::Application::Application;
        virtual ~Application();

        bool Init(bool interactive = true);
        bool Run();
        void TearDown();

    private:
        bool InitLog(const char* logName);
        bool RegisterEngine(bool interactive);

        AZStd::unique_ptr<PythonBindings> m_pythonBindings;
        QSharedPointer<QCoreApplication> m_app;
        QSharedPointer<ProjectManagerWindow> m_mainWindow;

        AZ::Entity* m_entity = nullptr;
    };
}
