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

#include <AzQtComponents/Application/ToolsApplication.h>

#include <AzCore/PlatformIncl.h> // This should be the first include to make sure Windows.h is defined with NOMINMAX

namespace AzQtComponents
{
    /*
    AZStd::string_view GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view { LY_CMAKE_TARGET };
    }
    */

     class ToolsApplication::Impl
         : private AZ::Debug::TraceMessageBus::Handler
         , public AzFramework::Application
    {
         friend class ToolsApplication;

    public:
         Impl(ToolsApplication* app) : m_app(app)
         {
             
         }
         ToolsApplication* m_app;

         bool OnOutput(const char* window, const char* message) override;         

     protected:  
         struct LogMessage
         {
             AZStd::string window;
             AZStd::string message;
         };

         AZStd::vector<LogMessage> m_startupLogSink;
         AZStd::unique_ptr<AzFramework::LogFile> m_logFile;

    };

    ToolsApplication::ToolsApplication(int& argc, char** argv)
        : QApplication(argc, argv)
        , m_impl(new Impl(this))
    {
         /*
         QApplication::setOrganizationName("Amazon");
         QApplication::setOrganizationDomain("amazon.com");
         QApplication::setApplicationName("O3DEToolsApplication");

         AzQtComponents::PrepareQtPaths();

         QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
 
    // Must be set before QApplication is initialized, so that we support HighDpi monitors, like the Retina displays
         // on Windows 10
         
         QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
         QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
         QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
         QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
         AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);
         */

         //m_impl->AZ::Debug::TraceMessageBus::Handler::BusConnect();

    }

    ToolsApplication::~ToolsApplication()
    {
        //m_impl->AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    
    bool ToolsApplication::Impl::OnOutput(const char* window, const char* message)
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

    /*
    bool ToolsApplication::AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end() || !widget)
        {
            return false;
        }

        auto dockWidget = new AzQtComponents::StyledDockWidget(name.c_str());
        dockWidget->setObjectName(QString("%1_DockWidget").arg(name.c_str()));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        widget->setObjectName(name.c_str());
        widget->setParent(dockWidget);
        widget->setMinimumSize(QSize(300, 300));
        dockWidget->setWidget(widget);
        //QMainWindow::addDockWidget(aznumeric_cast<Qt::DockWidgetArea>(area), dockWidget);
        //QMainWindow::resizeDocks({ dockWidget }, { 400 }, aznumeric_cast<Qt::Orientation>(orientation));
        m_dockWidgets[name] = dockWidget;
        return true;
    }

    void ToolsApplication::RemoveDockWidget(const AZStd::string& name)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            delete dockWidgetItr->second;
            m_dockWidgets.erase(dockWidgetItr);
        }
    }

    void ToolsApplication::SetDockWidgetVisible(const AZStd::string& name, bool visible)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            dockWidgetItr->second->setVisible(visible);
        }
    }

    bool ToolsApplication::IsDockWidgetVisible(const AZStd::string& name) const
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            return dockWidgetItr->second->isVisible();
        }
        return false;
    }

    AZStd::vector<AZStd::string> ToolsApplication::GetDockWidgetNames() const
    {
        AZStd::vector<AZStd::string> names;
        names.reserve(m_dockWidgets.size());
        for (const auto& dockWidgetPair : m_dockWidgets)
        {
            names.push_back(dockWidgetPair.first);
        }
        return names;
    }
    */
    
} // namespace AzQtComponents

