/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        /// Retrieve the main application window.
        virtual QWidget* GetAppMainWindow() { return nullptr; }

        ///Deactivate the Editor UI.
        virtual void DeactivateEditorUI( [[maybe_unused]] bool deactivate) { }
    };

    using EditorWindowRequestBus = AZ::EBus<EditorWindowRequests>;

    /// Helper for EditorWindowRequests to be used as a 
    /// member instead of inheriting from EBus directly.
    class EditorWindowRequestBusImpl
        : public EditorWindowRequestBus::Handler
    {
    public:
        /// Set the function to be called when entering ComponentMode.
        void SetDeactivateEditorUIFunc(const AZStd::function<void(bool)>& enteredDeactivateEditorUIFunc)
        {
            m_deactivateEditorUIFunc = enteredDeactivateEditorUIFunc;
        }

        private:
        // EditorWindowRequestBus
        void DeactivateEditorUI(bool deactivate) override
        {
            m_deactivateEditorUIFunc(deactivate);
        }

        AZStd::function<void(bool)> m_deactivateEditorUIFunc; ///< Function to call when entering ComponentMode.
    };

} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_EDITORWINDOWREQUESTBUS_H
