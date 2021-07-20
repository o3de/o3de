/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZTOOLSFRAMEWORK_EDITORWINDOWREQUESTBUS_H
#define AZTOOLSFRAMEWORK_EDITORWINDOWREQUESTBUS_H

#include <AzCore/base.h>

#pragma once

#include <AzCore/EBus/EBus.h>

class QWidget;

namespace AzToolsFramework
{
    /**
     * Bus for general editor window requests to be intercepted by the application.
     */
    class EditorWindowRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<EditorWindowRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Retrieve the main application window.
        virtual QWidget* GetAppMainWindow() { return nullptr; }
    };
    using EditorWindowRequestBus = AZ::EBus<EditorWindowRequests>;

    class EditorWindowUIRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorWindowUIRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        /// Enable/Disable the Editor UI.
        virtual void SetEditorUiEnabled([[maybe_unused]] bool enable) {}
    };
    using EditorWindowUIRequestBus = AZ::EBus<EditorWindowUIRequests>;

    using EnableUiFunction = AZStd::function<void(bool)>;

    /// Helper for EditorWindowRequests to be used as a 
    /// member instead of inheriting from EBus directly.
    class EditorWindowRequestBusImpl
        : public EditorWindowUIRequestBus::Handler
    {
    public:
        /// Set the function to be called when entering ImGui Mode.
        void SetEnableEditorUiFunc(const EnableUiFunction enableEditorUiFunc)
        {
            m_enableEditorUiFunc = AZStd::move(enableEditorUiFunc);
        }

        private:
        // EditorWindowRequestBus
        void SetEditorUiEnabled( [[maybe_unused]] bool enable) override
        {
            m_enableEditorUiFunc(enable);
        }

        EnableUiFunction m_enableEditorUiFunc; ///< Function to call when entering ImGui Mode.
    };

} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_EDITORWINDOWREQUESTBUS_H
