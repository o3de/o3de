/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Custom preference page interfaces

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IPREFERENCESPAGE_H
#define CRYINCLUDE_EDITOR_INCLUDE_IPREFERENCESPAGE_H
#pragma once
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

#endif // CRYINCLUDE_EDITOR_INCLUDE_IPREFERENCESPAGE_H
