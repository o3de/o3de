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
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Retrieve the main application window.
        virtual QWidget* GetAppMainWindow() { return nullptr; }
    };

    using EditorWindowRequestBus = AZ::EBus<EditorWindowRequests>;
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_EDITORWINDOWREQUESTBUS_H
