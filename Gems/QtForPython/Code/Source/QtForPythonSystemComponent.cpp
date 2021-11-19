/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QtForPythonSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // (qwidget.h) 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QPointer>
#include <QWidget>
#include <QApplication>
#include <QDateTime>
AZ_POP_DISABLE_WARNING

// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#pragma pop_macro("slots")

namespace QtForPython
{
    static const constexpr int s_loopTimerInterval = 5;
    static const constexpr float s_maxTime = 25.f * 60.f * 60.f;

    class QtForPythonEventHandler : public QObject
    {
    private:
        std::function<void()> m_loopCallback;
        float m_time = 0.f;
        QPointer<QObject> m_lastTimerParent = nullptr;
        int m_lastTimerId = 0;

    public:
        void SetupTimer(QObject* parent)
        {
            if (parent == m_lastTimerParent)
            {
                return;
            }
            if (m_lastTimerParent)
            {
                m_lastTimerParent->killTimer(m_lastTimerId);
            }
            m_lastTimerId = parent->startTimer(s_loopTimerInterval, Qt::CoarseTimer);
            m_lastTimerParent = parent;
        }

        QtForPythonEventHandler(QObject* parent = nullptr)
            : QObject(parent)
        {
            qApp->installEventFilter(this);
            SetupTimer(this);
        }

        float GetTime() const
        {
            return m_time;
        }

        void RunEventLoop()
        {
            if (m_loopCallback)
            {
                try
                {
                    m_loopCallback();
                }
                catch (pybind11::error_already_set& pythonError)
                {
                    // Release the exception stack and let Python print it
                    pythonError.restore();
                    PyErr_Print();
                }
            }
        }

        bool eventFilter(QObject* obj, QEvent* event)
        {
            // Determine which object should own our event loop timer
            // By default it's this object
            QObject* activeTimerParent = this;
            // If it's a modal or popup widget, use that to ensure we get timer events
            if (qApp->activePopupWidget())
            {
                activeTimerParent = qApp->activePopupWidget();
            }
            else if (qApp->activeModalWidget())
            {
                activeTimerParent = qApp->activeModalWidget();
            }
            SetupTimer(activeTimerParent);

            if (obj == m_lastTimerParent && event->type() == QEvent::Timer && static_cast<QTimerEvent*>(event)->timerId() == m_lastTimerId)
            {
                m_time += s_loopTimerInterval / 1000.f;
                if (m_time > s_maxTime) 
                {
                    m_time = 0.f;
                }
                RunEventLoop();
            }

            return false;
        }

        void SetLoopCallback(std::function<void()> callback)
        {
            m_loopCallback = callback;
        }

        void ClearLoopCallback()
        {
            m_loopCallback = {};
        }

        bool HasLoopCallback() const
        {
            return m_loopCallback.operator bool();
        }
    };

    void QtForPythonSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<QtForPythonSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            serialize->RegisterGenericType<QWidget>();
        }

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->EBus<QtForPythonRequestBus>("QtForPythonRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "qt")
                ->Event("IsActive", &QtForPythonRequestBus::Events::IsActive)
                ->Event("GetQtBootstrapParameters", &QtForPythonRequestBus::Events::GetQtBootstrapParameters)
                ;

            behavior->Class<QtBootstrapParameters>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "qt")
                ->Property("qtBinaryFolder", BehaviorValueProperty(&QtBootstrapParameters::m_qtBinaryFolder))
                ->Property("qtPluginsFolder", BehaviorValueProperty(&QtBootstrapParameters::m_qtPluginsFolder))
                ->Property("mainWindowId", BehaviorValueProperty(&QtBootstrapParameters::m_mainWindowId))
                ;
        }
    }

    void QtForPythonSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("QtForPythonService"));
    }

    void QtForPythonSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("QtForPythonService"));
    }

    void QtForPythonSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(EditorPythonBindings::PythonEmbeddedService);
    }

    void QtForPythonSystemComponent::Activate()
    {
        m_eventHandler = new QtForPythonEventHandler;
        QtForPythonRequestBus::Handler::BusConnect();
        EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusConnect();
    }

    void QtForPythonSystemComponent::Deactivate()
    {
        QtForPythonRequestBus::Handler::BusDisconnect();
        EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
        delete m_eventHandler;
    }

    bool QtForPythonSystemComponent::IsActive() const
    {
        return AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers();
    }

    QtBootstrapParameters QtForPythonSystemComponent::GetQtBootstrapParameters() const
    {
        QtBootstrapParameters params;
       
#if !defined(Q_OS_WIN)
#error Unsupported OS platform for this QtForPython gem
#endif

        params.m_mainWindowId = 0;
        using namespace AzToolsFramework;
        QWidget* activeWindow = nullptr;
        EditorWindowRequestBus::BroadcastResult(activeWindow, &EditorWindowRequests::GetAppMainWindow);
        if (activeWindow)
        {
            // store the Qt main window so that scripts can hook into the main menu and/or docking framework
            params.m_mainWindowId = aznumeric_cast<AZ::u64>(activeWindow->winId());
        }

        // prepare the folder where the build system placed the QT binary files
        AZ::ComponentApplicationBus::BroadcastResult(params.m_qtBinaryFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);

        // prepare the QT plugins folder
        AZ::StringFunc::Path::Join(params.m_qtBinaryFolder.c_str(), "EditorPlugins", params.m_qtPluginsFolder);
        return params;
    }

    void QtForPythonSystemComponent::OnImportModule(PyObject* module)
    {
        // Register azlmbr.qt_helpers for our event loop callback
        pybind11::module parentModule = pybind11::cast<pybind11::module>(module);
        std::string pythonModuleName = pybind11::cast<std::string>(parentModule.attr("__name__"));
        if (AzFramework::StringFunc::Equal(pythonModuleName.c_str(), "azlmbr"))
        {
            pybind11::module helperModule = parentModule.def_submodule("qt_helpers");
            helperModule.def("set_loop_callback", [this](std::function<void()> callback)
                {
                    if (m_eventHandler)
                    {
                        m_eventHandler->SetLoopCallback(callback);
                    }
                }, R"delim(
                Sets a callback that will be invoked periodically during the course of Qt's event loop (even if a nested event loop is running).
                This is intended for internal use in pyside_utils and should generally not be used directly.)delim");
            helperModule.def("clear_loop_callback", [this]()
                {
                    if (m_eventHandler)
                    {
                        m_eventHandler->ClearLoopCallback();
                    }
                }, R"delim(
                Clears callback that will be invoked periodically during the course of Qt's event loop.
                This is intended for internal use in pyside_utils and should generally not be used directly.)delim");
            helperModule.def("loop_is_running", [this]()
                {
                    if (m_eventHandler)
                    {
                        return m_eventHandler->HasLoopCallback();
                    }
                    return false;
                }, R"delim(
                Returns True if the qt_helper event_loop callback is set and running.
                This is intended for internal use in pyside_utils and should generally not be used directly.)delim");
            helperModule.def("time", [this]()
                {
                    if (m_eventHandler)
                    {
                        return m_eventHandler->GetTime();
                    }
                    return -1.f;
                }, R"delim(
                Returns a floating timestamp, measured in seconds, that updates with the Qt event loop.
                This is intended for internal use in pyside_utils and should generally not be used directly.)delim");
        }
    }
}
