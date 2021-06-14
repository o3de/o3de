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

        AZStd::unique_ptr<PythonBindings> m_pythonBindings;
        QSharedPointer<QCoreApplication> m_app;
        QSharedPointer<ProjectManagerWindow> m_mainWindow;

        AZ::Entity* m_entity = nullptr;
    };
}
