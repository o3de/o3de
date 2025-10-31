/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

struct ISystem;
struct SSystemInitParams;

/*!
 * Events from CrySystem
 */
class CrySystemEvents
    : public AZ::EBusTraits
{
public:
    //! ISystem has been created and is about to initialize.
    virtual void OnCrySystemPreInitialize(ISystem&, const SSystemInitParams&) {}

    //! ISystem and IConsole has been created but the cfg files have not been parsed
    virtual void OnCrySystemCVarRegistry() {}

    //! ISystem has been created and initialized.
    virtual void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) {}

    //! In-Editor systems have been created and initialized.
    virtual void OnCryEditorInitialized() {}

    //! Editor has started a level export
    virtual void OnCryEditorBeginLevelExport() {}

    //! Editor has finished a level export
    virtual void OnCryEditorEndLevelExport(bool /*success*/) {}

    //! ISystem is about to begin shutting down
    virtual void OnCrySystemShutdown(ISystem&) {}

    //! ISystem has shut down.
    virtual void OnCrySystemPostShutdown() {}

    //! Sent when a new level is being created.
    virtual void OnCryEditorBeginCreate() {}

    //! Sent after a new level has been created.
    virtual void OnCryEditorEndCreate() {}

    //! Sent when a level is about to be loaded.
    virtual void OnCryEditorBeginLoad() {}

    //! Sent after a level has been loaded.
    virtual void OnCryEditorEndLoad() {}

    //! Sent when the document is about to close.
    virtual void OnCryEditorCloseScene() {}

    //! Sent when the document is closed.
    virtual void OnCryEditorSceneClosed() {}
};
using CrySystemEventBus = AZ::EBus<CrySystemEvents>;

/*!
 * Requests to CrySystem
 */
class CrySystemRequests
    : public AZ::EBusTraits
{
public:
    //! Get CrySystem
    virtual ISystem* GetCrySystem() = 0;
};
using CrySystemRequestBus = AZ::EBus<CrySystemRequests>;

DECLARE_EBUS_EXTERN(CrySystemRequests);
