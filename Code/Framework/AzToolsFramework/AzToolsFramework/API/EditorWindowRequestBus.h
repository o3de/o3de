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
