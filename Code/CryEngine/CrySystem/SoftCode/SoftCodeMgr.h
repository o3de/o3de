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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYSYSTEM_SOFTCODE_SOFTCODEMGR_H
#define CRYINCLUDE_CRYSYSTEM_SOFTCODE_SOFTCODEMGR_H
#pragma once

#include <CryListenerSet.h>

#include "ISoftCodeMgr.h"

struct ITypeLibrary;
struct ITypeRegistrar;
class CExchanger;

// Internal: Performs the dynamic type management needed for SoftCoding
class DynamicTypeLibrary
    : public ITypeLibrary
{
public:
    DynamicTypeLibrary(const char* name = NULL);

    // ITypeLibrary impl.
    virtual const char* GetName();
    virtual void* CreateInstanceVoid(const char* typeName);
    virtual void SetOverride(ITypeLibrary* pOverrideLib);
    virtual size_t GetTypes(ITypeRegistrar** ppRegistrar, size_t& count) const;

    void AddListener(const char* libraryName, ISoftCodeListener* pListener, const char* listenerName);
    void RemoveListener(ISoftCodeListener* pListener);

    // Attempts to add the library types the active set
    void IntegrateLibrary(ITypeLibrary* pLib, bool isDefault);

    ITypeRegistrar* FindTypeForInstance(void* pInstance) const;

private:
    // Attempts to add the type the active set
    void IntegrateType(ITypeRegistrar* pType, bool isDefault);
    // Ensure the type can be safely created, visited, destroyed and prep exchanger
    bool EvaluateType(ITypeRegistrar* pType, CExchanger& exchanger);

private:
    typedef std::vector<ITypeLibrary*> TLibVec;
    typedef std::map<string, ITypeRegistrar*> TTypeMap;
    typedef CListenerSet<ISoftCodeListener*> TListeners;

    // The current set of active types
    std::map<string, ITypeRegistrar*> m_types;
    // Current set of loaded libraries
    TLibVec m_history;
    // Set of listeners to SC changes
    TListeners m_listeners;

    const char* m_name;     // Supplied by the first real library that registers
};


// Implements the global singleton responsible for SoftCode management
class SoftCodeMgr
    : public ISoftCodeMgr
{
public:
    SoftCodeMgr();
    virtual ~SoftCodeMgr();

    // Used to register built-in libraries on first use
    virtual void RegisterLibrary(ITypeLibrary* pLib);

    // Look for new SoftCode modules and load them, adding their types to the registry
    virtual void LoadNewModules();

    virtual void AddListener(const char* libraryName, ISoftCodeListener* pListener, const char* listenerName);
    virtual void RemoveListener(const char* libraryName, ISoftCodeListener* pListener);

    // To be called regularly to poll for library updates
    virtual void PollForNewModules();

    // Stops thread execution until a new SoftCode module is available
    virtual void* WaitForUpdate(void* pInstance);

private:
    bool LoadModule(const char* moduleName);
    size_t FindSoftCodeFiles(const string& searchName, std::vector<string>& foundPaths) const;

private:
    typedef std::map<string, DynamicTypeLibrary> TLibMap;
    typedef std::set<string> TLoadedLibSet;

    // Records the history for each TypeLibrary keyed by library name
    TLibMap m_libraryMap;

    // Records the library files already loaded
    TLoadedLibSet m_loadedSet;

    // Used to determine when the next auto-update will occur
    CTimeValue m_nextAutoCheckTime;
};


#endif // CRYINCLUDE_CRYSYSTEM_SOFTCODE_SOFTCODEMGR_H
