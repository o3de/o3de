/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
#ifndef CRYINCLUDE_EDITOR_PLUGIN_H
#define CRYINCLUDE_EDITOR_PLUGIN_H
#include "Include/IEditorClassFactory.h"

#include "Util/GuidUtil.h"

//! Derive from this class to decrease the amount of work for creating a new class description
//! Provides standard reference counter implementation for IUnknown
class CRefCountClassDesc
    : public IClassDesc
{
public:
    virtual ~CRefCountClassDesc() { }
    HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]] const IID& riid, [[maybe_unused]] void** ppvObj)
    {
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        ++m_nRefCount;
        return m_nRefCount;
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        int refs = m_nRefCount;

        if (--m_nRefCount <= 0)
        {
            delete this;
        }

        return refs;
    }

private:
    int m_nRefCount;
};


// Use this for debugging unregistration problems.
//#define DEBUG_CLASS_NAME_REGISTRATION


//! Class factory is a common repository of all registered plugin classes,
//! Classes here can found by their class ID or all classes of given system class retrieved
class CRYEDIT_API CClassFactory
    : public IEditorClassFactory
{
public:
    CClassFactory();
    ~CClassFactory();

    //! Access class factory singleton.
    static CClassFactory* Instance();
    //! Register a new class to the factory
    void RegisterClass(IClassDesc* pClassDesc);
    //! Find class in the factory by class name
    IClassDesc* FindClass(const char* className) const;
    //! Find class in the factory by class ID
    IClassDesc* FindClass(const GUID& rClassID) const;
    //! Find View Pane Class in the factory by pane title
    IViewPaneClass* FindViewPaneClassByTitle(const char* pPaneTitle) const;
    void UnregisterClass(const char* pClassName);
    void UnregisterClass(const GUID& rClassID);
    //! Get classes matching specific requirements ordered alphabetically by name.
    void GetClassesBySystemID(ESystemClassID aSystemClassID, std::vector<IClassDesc*>& rOutClasses);
    void GetClassesByCategory(const char* pCategory, std::vector<IClassDesc*>& rOutClasses);

private:
    void RegisterAutoTypes();

    typedef std::map<QString, IClassDesc*> TNameMap;
    typedef std::map<GUID, IClassDesc*, guid_less_predicate> TGuidMap;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    TNameMap m_nameToClass;
    TGuidMap m_guidToClass;
    std::vector<IClassDesc*> m_classes;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#if defined(DEBUG_CLASS_NAME_REGISTRATION)
    // This vector will mirror that of CClassFactory::m_classes.
    // When a class description is destroyed without being unregistered first
    // (i.e. from within plugins is a common scenario) you will get a crash
    // in ~CClassFactory as it's Releasing m_classes.
    // When you hit that crash, take the index and look into this vector at
    // the class name.  That class did not unregister before it was released.
    std::vector<string> m_debugClassNames;
#endif // DEBUG_CLASS_NAME_REGISTRATION

    static CClassFactory* s_pInstance;
};

//! Auto registration for classes
class CAutoRegisterClassHelper
{
public:
    CAutoRegisterClassHelper(IClassDesc* pClassDesc)
    {
        m_pClassDesc = pClassDesc;
        m_pNext = 0;

        if (!s_pLast)
        {
            s_pFirst = this;
        }
        else
        {
            s_pLast->m_pNext = this;
        }

        s_pLast = this;
    }

    IClassDesc* m_pClassDesc;
    CAutoRegisterClassHelper* m_pNext;
    static CAutoRegisterClassHelper* s_pFirst;
    static CAutoRegisterClassHelper* s_pLast;
};

// Use this define to automatically register a new class description.
#define REGISTER_CLASS_DESC(ClassDesc) \
    CAutoRegisterClassHelper g_AutoRegHelper##ClassDesc(new ClassDesc);

#define REGISTER_QT_CLASS_DESC(ClassDesc, name, category) \
    CAutoRegisterClassHelper g_AutoRegHelper##ClassDesc(new CQtViewClass<ClassDesc>(name, category));

#define REGISTER_QT_CLASS_DESC_SYSTEM_ID(ClassDesc, name, category, systemid) \
    CAutoRegisterClassHelper g_AutoRegHelper##ClassDesc(new CQtViewClass<ClassDesc>(name, category, systemid));

#endif // CRYINCLUDE_EDITOR_PLUGIN_H
