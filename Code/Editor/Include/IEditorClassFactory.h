/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Class factory support classes


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IEDITORCLASSFACTORY_H
#define CRYINCLUDE_EDITOR_INCLUDE_IEDITORCLASSFACTORY_H
#pragma once

#include <CryCommon/platform.h>
#include <vector>
#include <QtCore/QString>
#include <AzCore/Math/Guid.h>

#define DEFINE_UUID(l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
static const GUID uuid() { return { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }; }

#ifdef AZ_PLATFORM_WINDOWS
#include <Unknwn.h>
#else
struct IUnknown
{
    virtual ~IUnknown() = default;
};
#endif

#ifdef __uuidof
#undef __uuidof
#endif
#define __uuidof(T) T::uuid()

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)

# ifndef _REFGUID_DEFINED
# define _REFGUID_DEFINED
typedef const GUID& REFGUID;
# endif

# ifndef _REFIID_DEFINED
# define _REFIID_DEFINED
typedef const GUID& REFIID;
# endif

# ifndef IID_DEFINED
# define IID_DEFINED
typedef GUID IID;
# endif

#ifndef HRESULT_VALUES_DEFINED
#define HRESULT_VALUES_DEFINED
enum
{
    E_OUTOFMEMORY  = 0x8007000E,
    E_FAIL         = 0x80004005,
    E_ABORT        = 0x80004004,
    E_INVALIDARG   = 0x80070057,
    E_NOINTERFACE  = 0x80004002,
    E_NOTIMPL      = 0x80004001,
    E_UNEXPECTED   = 0x8000FFFF
};
#endif

#endif  // defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)

#include "SandboxAPI.h"

class QObject;


//! System class IDs
enum ESystemClassID
{
    ESYSTEM_CLASS_OBJECT = 0x0001,
    ESYSTEM_CLASS_EDITTOOL = 0x0002,
    ESYSTEM_CLASS_PREFERENCE_PAGE = 0x0020,
    ESYSTEM_CLASS_VIEWPANE = 0x0021,
    //! Source/Asset Control Management Provider
    ESYSTEM_CLASS_SCM_PROVIDER = 0x0022,
    ESYSTEM_CLASS_CONSOLE_CONNECTIVITY = 0x0023,
    ESYSTEM_CLASS_ASSET_DISPLAY = 0x0024,
    ESYSTEM_CLASS_ASSET_TAGGING = 0x0025,
    ESYSTEM_CLASS_FRAMEWND_EXTENSION_PANE = 0x0030,
    ESYSTEM_CLASS_TRACKVIEW_KEYUI = 0x0040,
    ESYSTEM_CLASS_UITOOLS = 0x0050, // UI Emulator tool
    ESYSTEM_CLASS_CONTROL = 0x0900,
    ESYSTEM_CLASS_USER = 0x1000
};

//! This interface describes a class created by a plugin
struct IClassDesc
    : public IUnknown
{
    //////////////////////////////////////////////////////////////////////////
    // IUnknown implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]] const IID& riid, [[maybe_unused]] void** ppvObj) { return E_NOINTERFACE; }
    virtual ULONG STDMETHODCALLTYPE AddRef() { return 0; }
    virtual ULONG STDMETHODCALLTYPE Release() { return 0; }

    template<class Q>
    HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
    {
        return QueryInterface(__uuidof(Q), (void**)pp);
    }

    //////////////////////////////////////////////////////////////////////////
    // Class description.
    //////////////////////////////////////////////////////////////////////////
    //! This method returns an Editor defined GUID describing the class this plugin class is associated with.
    virtual ESystemClassID SystemClassID() = 0;
    //! Return the GUID of the class created by plugin.
    virtual REFGUID ClassID() = 0;
    //! This method returns the human readable name of the class.
    virtual QString ClassName() = 0;
    //! This method returns Category of this class, Category is specifying where this plugin class fits best in
    //! create panel.
    virtual QString Category() = 0;

    virtual QString MenuSuggestion() { return QString(); }
    virtual QString Tooltip() { return QString(); }
    virtual QString Description() { return QString(); }

    //! This method returns if the plugin should have a menu item for its pane.
    virtual bool ShowInMenu() const { return true; }
    //! Qt equivalent of CRuntimeClass::CreateObject(). We might create a full QRuntimeClass, if there's a need for it.
    virtual QObject* CreateQObject() const { return nullptr; }

    //! For any class that may be conditionally enabled or disabled, this function can be overriden to return true if it is enabled, false otherwise.
    //! The default is to always return true.
    virtual bool IsEnabled() const { return true; }
    //////////////////////////////////////////////////////////////////////////
};


struct IViewPaneClass;

struct CRYEDIT_API IEditorClassFactory
{
public:
    virtual ~IEditorClassFactory() = default;

    //! Register new class to the factory.
    virtual void RegisterClass(IClassDesc* pClassDesc) = 0;
    //! Find class in the factory by class name.
    virtual IClassDesc* FindClass(const char* pClassName) const = 0;
    //! Find class in the factory by class id
    virtual IClassDesc* FindClass(const GUID& rClassID) const = 0;
    virtual IViewPaneClass* FindViewPaneClassByTitle(const char* pPaneTitle) const = 0;
    virtual void UnregisterClass(const char* pClassName) = 0;
    virtual void UnregisterClass(const GUID& rClassID) = 0;
    //! Get classes that matching specific requirements.
    virtual void GetClassesBySystemID(ESystemClassID aSystemClassID, std::vector<IClassDesc*>& rOutClasses) = 0;
    virtual void GetClassesByCategory(const char* pCategory, std::vector<IClassDesc*>& rOutClasses) = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IEDITORCLASSFACTORY_H
