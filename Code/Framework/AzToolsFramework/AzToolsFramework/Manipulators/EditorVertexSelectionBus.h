/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    //! Bus to request operations to the EditorVertexSelection classes.
    class EditorVertexSelectionRequests : public AZ::EBusTraits
    {
    public:
        virtual void DuplicateSelectedVertices() {};
        virtual void DeleteSelectedVertices() {};
        virtual void ClearVertexSelection() = 0;

    protected:
        ~EditorVertexSelectionRequests() = default;
    };
    using EditorVertexSelectionRequestBus = AZ::EBus<EditorVertexSelectionRequests>;

    //! Bus to be notified about changes in Editor Vertex Selection state.
    class EditorVertexSelectionNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnEditorVertexSelectionChange() {}

    protected:
        ~EditorVertexSelectionNotifications() = default;
    };
    using EditorVertexSelectionNotificationBus = AZ::EBus<EditorVertexSelectionNotifications>;
}
