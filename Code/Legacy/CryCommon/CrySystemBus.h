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
 *  use AzToolsFramework::EditorSystemEvents instead
 */
class CrySystemEvents
    : public AZ::EBusTraits
{
public:
    //! ISystem has been created and is about to initialize.
    // [[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorSystemPreInitialize instead")]]
    virtual void OnCrySystemPreInitialize(ISystem&, const SSystemInitParams&) {}

    //! ISystem and IConsole has been created but the cfg files have not been parsed
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorSystemCVarRegistry instead")]]
    virtual void OnCrySystemCVarRegistry() {}

    //! ISystem has been created and initialized.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorSystemInitialized instead")]]
    virtual void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) {}

    //! In-Editor systems have been created and initialized.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorInitialized instead")]]
    virtual void OnCryEditorInitialized() {}

    //! Editor has started a level export
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorBeginLevelExport instead")]]
    virtual void OnCryEditorBeginLevelExport() {}

    //! Editor has finished a level export
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorEndLevelExport instead")]]
    virtual void OnCryEditorEndLevelExport(bool /*success*/) {}

    //! ISystem is about to begin shutting down
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorSystemPreShutdown instead")]]
    virtual void OnCrySystemShutdown(ISystem&) {}

    //! ISystem has shut down.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorSystemShutdown instead")]]
    virtual void OnCrySystemPostShutdown() {}

    //! Sent when a new level is being created.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorBeginCreate instead")]]
    virtual void OnCryEditorBeginCreate() {}

    //! Sent after a new level has been created.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorEndCreate instead")]]
    virtual void OnCryEditorEndCreate() {}

    //! Sent when a level is about to be loaded.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorBeginLoad instead")]]
    virtual void OnCryEditorBeginLoad() {}

    //! Sent after a level has been loaded.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorEndLoad instead")]]
    virtual void OnCryEditorEndLoad() {}

    //! Sent when the document is about to close.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorCloseScene instead")]]
    virtual void OnCryEditorCloseScene() {}

    //! Sent when the document is closed.
    //[[deprecated("use AzToolsFramework::EditorSystemRequestBus::OnEditorSceneClosed instead")]]
    virtual void OnCryEditorSceneClosed() {}
};

using CrySystemEventBus = AZ::EBus<CrySystemEvents>;

/*!
 * Requests to CrySystem
 *  use AzToolsFramework::EditorSystemRequests instead
 */
class CrySystemRequests
    : public AZ::EBusTraits
{
public:
    //! Get CrySystem
    //[[deprecated("use AzToolsFramework::EditorSystemRequests::GetEditorSystem")]]
    virtual ISystem* GetCrySystem() = 0;
};

using CrySystemRequestBus = AZ::EBus<CrySystemRequests>;

DECLARE_EBUS_EXTERN(CrySystemRequests);
