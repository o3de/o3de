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

#include <QApplication>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzCore/Debug/TraceMessageBus.h>


namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API AzQtApplication
        : public QApplication
    {
    public:       
        AzQtApplication(int& argc, char** argv);
        ~AzQtApplication();
       

    private:
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        class Impl;
        AZStd::unique_ptr<Impl> m_impl;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };

    class LogMessage
    {
    public:
        AZStd::string window;
        AZStd::string message;
    };
} // namespace AzQtComponents



