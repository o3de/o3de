/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <CryCommon/ISystem.h> // Only until ISystem can be ported to non-Cry

struct ISystem;
struct SSystemInitParams;

namespace AzToolsFramework
{
    /*!
    * Events from EditorSystem
    */
    class EditorSystemEvents
        : public AZ::EBusTraits
    {
    public:
        //! ISystem has been created and is about to initialize.
        virtual void OnEditorSystemPreInitialize(ISystem&, const SSystemInitParams&) {}

        //! ISystem and IConsole has been created but the cfg files have not been parsed
        virtual void OnEditorSystemCVarRegistry() {}

        //! ISystem has been created and initialized.
        virtual void OnEditorSystemInitialized(ISystem&, const SSystemInitParams&) {}

        //! In-Editor systems have been created and initialized.
        virtual void OnEditorInitialized() {}

        //! Editor has started a level export
        virtual void OnEditorBeginLevelExport() {}

        //! Editor has finished a level export
        virtual void OnEditorEndLevelExport(bool /*success*/) {}

        //! ISystem is about to begin shutting down
        virtual void OnEditorSystemShutdown(ISystem&) {}

        //! ISystem has shut down.
        virtual void OnEditorSystemPostShutdown() {}

        //! Sent when a new level is being created.
        virtual void OnEditorBeginCreate() {}

        //! Sent after a new level has been created.
        virtual void OnEditorEndCreate() {}

        //! Sent when a level is about to be loaded.
        virtual void OnEditorBeginLoad() {}

        //! Sent after a level has been loaded.
        virtual void OnEditorEndLoad() {}

        //! Sent when the document is about to close.
        virtual void OnEditorCloseScene() {}

        //! Sent when the document is closed.
        virtual void OnEditorSceneClosed() {}
    };
    using EditorSystemEventBus = AZ::EBus<EditorSystemEvents>;

    /*!
    * Requests to EditorSystem
    */
    class EditorSystemRequests
        : public AZ::EBusTraits
    {
    public:
        //! Get EditorSystem
        virtual ISystem* GetEditorSystem() = 0;
    };
    using EditorSystemRequestBus = AZ::EBus<EditorSystemRequests>;

}
DECLARE_EBUS_EXTERN(AzToolsFramework::EditorSystemRequests);
