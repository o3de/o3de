/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Custom preference page interfaces

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IPREFERENCESPAGE_H
#define CRYINCLUDE_EDITOR_INCLUDE_IPREFERENCESPAGE_H
#pragma once
#include "Plugin.h"
#include <AzCore/RTTI/RTTI.h>

//! The interface class for preferences pages.
struct IPreferencesPage
{
    AZ_RTTI(IPreferencesPage, "{DEB112AD-55AD-4407-8482-BDA095A64752}")

    //! Return category where this preferences page belongs.
    virtual const char* GetCategory() = 0;
    //! Title of this preferences page.
    virtual const char* GetTitle() = 0;
    //! Return the icon for this page.
    virtual QIcon& GetIcon() = 0;
    //! Called by the editor when the Apply Now button is clicked.
    virtual void OnApply() = 0;
    //! Called by the editor when the Cancel button is clicked.
    virtual void OnCancel() = 0;
    //! Called by the editor when the Cancel button is clicked, and before the cancel has taken place.
    //! @return true to perform Cancel operation, false to abort Cancel.
    virtual bool OnQueryCancel() = 0;
    //! Called by the editor when the preferences page is made the active page or is not longer the active page.
    //! @param bActive true when page become active, false when page deactivated.
};

//! Interface used to create new preferences pages.
//! You can query this interface from any IClassDesc interface with ESYSTEM_CLASS_PREFERENCE_PAGE system class Id.
struct IPreferencesPageCreator
{
    DEFINE_UUID(0xD494113C, 0xBF13, 0x4171, 0x91, 0x71, 0x03, 0x33, 0xDF, 0x10, 0xEA, 0xFC)

    //! Get number of preferences page hosted by this class.
    virtual int GetPagesCount() = 0;
    //! Creates a new preferences page by page index.
    //! @param index must be within 0 <= index < GetPagesCount().
    virtual IPreferencesPage* CreateEditorPreferencesPage(int index) = 0;
};

//! A plugin class description for all IPreferencesPage derived classes.
struct IPreferencesPageClassDesc
    : public IClassDesc
{
    //////////////////////////////////////////////////////////////////////////
    // IClassDesc implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_PREFERENCE_PAGE; };
    //! This method returns the human readable name of the class.
    virtual QString ClassName() { return "Preferences Page"; };
    //! This method returns Category of this class,
    //! Category is specifying where this plugin class fits best in the create panel.
    virtual QString Category() { return "Preferences"; };
    //! Show a modal about dialog / message box for the plugin.
    virtual void ShowAbout() {};
    virtual bool CanExitNow() { return true; };
    //! The plugin should write / read its data to the passed stream. The data is saved to or loaded
    //! from the editor project file. This function is called during the usual save / load process of
    //! the editor's project file
    virtual void Serialize([[maybe_unused]] CXmlArchive& ar) {};
};
#endif // CRYINCLUDE_EDITOR_INCLUDE_IPREFERENCESPAGE_H
