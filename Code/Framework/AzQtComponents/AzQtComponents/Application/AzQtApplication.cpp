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

#include <AzQtComponents/Application/AzQtApplication.h>

#include <AzCore/PlatformIncl.h> // This should be the first include to make sure Windows.h is defined with NOMINMAX

namespace AzQtComponents
{
     class AzQtApplication::Impl
         : private AZ::Debug::TraceMessageBus::Handler
    {
         friend class AzQtApplication;

    public:
         Impl(AzQtApplication* app) : m_app(app)
         {
             
         }
         AzQtApplication* m_app;

         //////////////////////////////////////////////////////////////////////////
         // AZ::Debug::TraceMessageBus::Handler overrides...
         bool OnOutput(const char* window, const char* message) override;
         //////////////////////////////////////////////////////////////////////////      

     protected:  
         AZStd::vector<LogMessage> m_startupLogSink;
         AZStd::unique_ptr<AzFramework::LogFile> m_logFile;

    };

    AzQtApplication::AzQtApplication(int& argc, char** argv)
        : QApplication(argc, argv)
        , m_impl(new Impl(this))
    {
         // Use a common Qt settings path for applications that don't register their own application name
         if (QApplication::applicationName().isEmpty())
         {
            QApplication::setOrganizationName("Amazon");
            QApplication::setOrganizationDomain("amazon.com");
            QApplication::setApplicationName("O3DEToolsApplication");
         }

         AzQtComponents::PrepareQtPaths();

         QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
 
         // Must be set before QApplication is initialized, so that we support HighDpi monitors, like the Retina displays
             // on Windows 10
         
         QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
         QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
         QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
         QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
         AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);
         

         m_impl->AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    AzQtApplication::~AzQtApplication()
    {
        m_impl->AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    
    bool AzQtApplication::Impl::OnOutput(const char* window, const char* message)
    {
        // Suppress spam from the Source Control system
        constexpr char sourceControlWindow[] = "Source Control";

        if (0 == strncmp(window, sourceControlWindow, AZ_ARRAY_SIZE(sourceControlWindow)))
        {
            return true;
        }

        if (m_logFile)
        {
            m_logFile->AppendLog(AzFramework::LogFile::SEV_NORMAL, window, message);
        }
        else
        {
            m_startupLogSink.push_back({ window, message });
        }
        return false;
    }
    
} // namespace AzQtComponents

