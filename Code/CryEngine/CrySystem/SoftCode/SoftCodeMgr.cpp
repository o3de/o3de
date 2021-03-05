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

#include "CrySystem_precompiled.h"

#ifdef SOFTCODE_SYSTEM_ENABLED

#ifndef SOFTCODE_ENABLED
// Even if this module isn't built with SC enabled, if the SC system is enabled we define
// it for this compilation unit to ensure we use the correct versions of the IType* interfaces.
    #define SOFTCODE_ENABLED
#endif

#include "SoftCodeMgr.h"
#include <IConsole.h>
#include <CryLibrary.h>
#include <CryPath.h>
#include <AzCore/std/functional.h> // for function<> in find files

// This should resolve to "GetTypeLibrary" but we export by ordinal to avoid overheads on 360
// and keep everything consistent.
static const char* DLL_GETTYPELIBRARY = (LPCSTR)1;

struct CInstanceData
{
    CInstanceData(void* pInstance, size_t memberCount)
        : m_pOldInstance(pInstance)
        , m_pNewInstance()
    {
        m_members.resize(memberCount);
    }

    ~CInstanceData()
    {
        // Delete all members
        for (TMemberVec::iterator iter(m_members.begin());
             iter != m_members.end();
             ++iter)
        {
            // TODO: Safe cross module? Same allocator? Use a Destroy() method?
            delete *iter;
        }
    }

    void* Instance() { return m_pOldInstance; }

    void AddMember(size_t index, IExchangeValue& value)
    {
        // TODO: Add support for members with same name at different hierarchy levels
        assert(m_members[index] == NULL);
        assert(index != ~0);

        // Support expansion of m_members during while resolving members
        if (index >= m_members.size())
        {
            m_members.resize(index + 1);
        }

        m_members[index] = value.Clone();
    }

    IExchangeValue* GetMember(size_t index) const
    {
        assert(index < m_members.size());
        return m_members[index];
    }

    void SetNewInstance(void* pNewInstance) { m_pNewInstance = pNewInstance; }

    void* m_pOldInstance;
    void* m_pNewInstance;

    typedef std::vector<IExchangeValue*> TMemberVec;
    TMemberVec m_members;
};


class CExchanger
    : public IExchanger
{
public:
    CExchanger()
        : m_pInstanceData()
        , m_instanceIndex(~0)
        , m_state(eState_ResolvingMembers)
    {}

    virtual ~CExchanger()
    {
        DestroyInstanceData();
    }

    virtual bool IsLoading() const { return m_state >= eState_WritingNewMembers; }
    virtual size_t InstanceCount() const { return m_instances.size(); }

    virtual bool BeginInstance(void* pInstance)
    {
        if (IsLoading())
        {
            if (++m_instanceIndex < m_instances.size())
            {
                m_pInstanceData = m_instances[m_instanceIndex];
                m_pInstanceData->SetNewInstance(pInstance);
            }
            else
            {
                m_pInstanceData = NULL;
            }
        }
        else    // Reading/resolving members
        {
            m_instanceIndex = m_instances.size();
            m_pInstanceData = new CInstanceData(pInstance, m_memberMap.size());
            m_instances.push_back(m_pInstanceData);
        }

        return m_pInstanceData != NULL;
    }

    virtual bool SetValue(const char* name, IExchangeValue& value)
    {
        assert(!IsLoading());

        const size_t index = FindMemberIndex(name);
        const bool consumingValue = index != ~0;

        if (consumingValue)
        {
            m_pInstanceData->AddMember(index, value);
        }

        return consumingValue;
    }

    virtual IExchangeValue* GetValue(const char* name, void* pTarget, size_t targetSize)
    {
        assert(IsLoading());

        const size_t index = FindMemberIndex(name);

        // If member resolved (may not be if restoring to old instances)
        if (index != ~0)
        {
            // If member data available (may not be if member is new)
            if (IExchangeValue* pValue = m_pInstanceData->GetMember(index))
            {
                if (pValue->GetSizeOf() == targetSize)
                {
                    return pValue;
                }
                else    // Member size mismatch
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "SoftCode:  Member %s of instance %p has changed size (old: %d new: %d), setting to default value.",
                        name, m_pInstanceData->Instance(), (int)pValue->GetSizeOf(), (int)targetSize);
                }
            }
            else    // Member unknown
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "SoftCode:  Member %s (of instance %p) appears to be new.",
                    name, m_pInstanceData->Instance());

                // TODO: Could attempt to validate against a known wipe pattern ie. 0xfefefefe
                // This could catch most uninitialized variables...

                if (targetSize <= sizeof(void*))
                {
                    switch (targetSize)
                    {
                    case 1:
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "\tLeaving as: %d", *reinterpret_cast<char*>(pTarget));
                        break;
                    case 2:
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "\tLeaving as: %04x", *reinterpret_cast<short*>(pTarget));
                        break;
                    case 4:
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "\tLeaving as: %08x", *reinterpret_cast<int*>(pTarget));
                        break;
                    case 8:
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "\tLeaving as: %llx", *reinterpret_cast<long long*>(pTarget));
                        break;
                    }
                }
            }
        }

        // Indicate value should be default constructed
        return NULL;
    }

    // Used once required members have been established, members not already
    // encountered will be ignored.
    void LockMemberSet()
    {
        assert(m_state == eState_ResolvingMembers);

        DestroyInstanceData();
        m_state = eState_ReadingOldMembers;
    }

    // Rewinds instance data and prepare for loading
    void RewindForLoading()
    {
        assert(m_state == eState_ReadingOldMembers);

        m_pInstanceData = NULL;
        m_instanceIndex = ~0;
        m_state = eState_WritingNewMembers;
    }

    // Rewinds instance data to prepare to restore old members (UNDO)
    void RewindForRestore()
    {
        assert(m_state == eState_WritingNewMembers);

        m_pInstanceData = NULL;
        m_instanceIndex = ~0;
        m_state = eState_RestoringOldMembers;
    }

    void NotifyListenerOfReplacements(ISoftCodeListener* pListener)
    {
        for (TInstanceVec::const_iterator iter(m_instances.begin()); iter != m_instances.end(); ++iter)
        {
            CInstanceData* pInstanceData = *iter;
            pListener->InstanceReplaced(pInstanceData->m_pOldInstance, pInstanceData->m_pNewInstance);
        }
    }

private:
    void DestroyInstanceData()
    {
        m_pInstanceData = NULL;
        m_instanceIndex = ~0;

        for (TInstanceVec::iterator iter(m_instances.begin()); iter != m_instances.end(); ++iter)
        {
            delete *iter;
        }

        m_instances.resize(0);
    }

    inline size_t FindMemberIndex(const string& memberName)
    {
        size_t index = ~0;

        // If needed members have been resolved
        if (m_state != eState_ResolvingMembers)
        {
            TMemberMap::const_iterator iter(m_memberMap.find(memberName));
            if (iter != m_memberMap.end())
            {
                index = iter->second;
            }
        }
        else    // Add this member to the map with a new index
        {
            // Ensure there's no member name duplicates
            assert(m_memberMap.find(memberName) == m_memberMap.end());

            // A new entry
            index = m_memberMap.size();
            size_t& newIndex = m_memberMap[memberName];
            newIndex = index;
        }

        return index;
    }

private:
    CInstanceData* m_pInstanceData;
    size_t m_instanceIndex;

    typedef std::vector<CInstanceData*> TInstanceVec;
    TInstanceVec m_instances;

    // Maps instance members to offsets in instance member vectors
    typedef std::map<string, size_t> TMemberMap;
    TMemberMap m_memberMap;

    enum EState
    {
        eState_ResolvingMembers = 0,    // Record new member names as found
        eState_ReadingOldMembers,           // Scrape requested member data from old instances
        eState_WritingNewMembers,           // Write old member data to new instances
        eState_RestoringOldMembers,     // Restore scraped values to old instances (UNDO)
    };

    EState m_state;
};


// ----

DynamicTypeLibrary::DynamicTypeLibrary(const char* name)
    : m_name(name)
    , m_listeners(1)
{}

const char* DynamicTypeLibrary::GetName()
{
    return m_name;
}

void* DynamicTypeLibrary::CreateInstanceVoid(const char* typeName)
{
    TTypeMap::const_iterator typeIter(m_types.find(typeName));
    if (typeIter != m_types.end())
    {
        ITypeRegistrar* pRegistrar = typeIter->second;
        return pRegistrar->CreateInstance();
    }

    return NULL;
}

void DynamicTypeLibrary::SetOverride(ITypeLibrary* /*pOverrideLib*/)
{
    CryFatalError("Unsupported: Attempting to SetOverride on a DynamicTypeLibrary!");
}

size_t DynamicTypeLibrary::GetTypes([[maybe_unused]] ITypeRegistrar** ppRegistrar, [[maybe_unused]] size_t& count) const
{
    CryFatalError("Unsupported: Attempting to GetTypes on a DynamicTypeLibrary!");
    return 0;
}

void DynamicTypeLibrary::AddListener(const char* libraryName, ISoftCodeListener* pListener, const char* listenerName)
{
    // This DynamicTypeLibrary could have been created by this listener request
    // So ensure we have a name...!
    if (!m_name)
    {
        m_name = libraryName;
    }

    m_listeners.Add(pListener, listenerName);
}

void DynamicTypeLibrary::RemoveListener(ISoftCodeListener* pListener)
{
    m_listeners.Remove(pListener);
}

void DynamicTypeLibrary::IntegrateLibrary(ITypeLibrary* pLib, bool isDefault)
{
    typedef std::vector<ITypeRegistrar*> TTypeVec;

    // Resolve our name if we haven't already
    if (!m_name)
    {
        m_name = pLib->GetName();
    }

    // Override the new lib immediately
    pLib->SetOverride(this);

    // Query the new library for its types
    size_t typeCount = 0;
    pLib->GetTypes(NULL, typeCount);
    if (typeCount > 0)
    {
        TTypeVec typeVec;
        typeVec.resize(typeCount);
        pLib->GetTypes(&(typeVec.front()), typeCount);

        if (!isDefault)
        {
            CryLogAlways("SoftCode: Integrating %d new types defined in %s...", (int)typeCount, m_name);
        }

        // Attempt to integrate each type found
        for (TTypeVec::iterator typeIter(typeVec.begin()); typeIter != typeVec.end(); ++typeIter)
        {
            IntegrateType(*typeIter, isDefault);
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "SoftCode: New %s library has no registered types. Nothing to integrate.", pLib->GetName());
    }
}

ITypeRegistrar* DynamicTypeLibrary::FindTypeForInstance(void* pInstance) const
{
    for (TTypeMap::const_iterator iter(m_types.begin()); iter != m_types.end(); ++iter)
    {
        ITypeRegistrar* pType = iter->second;
        if (pType->HasInstance(pInstance))
        {
            return pType;
        }
    }

    return NULL;
}

void DynamicTypeLibrary::IntegrateType(ITypeRegistrar* pType, bool isDefault)
{
    const char* typeName = pType->GetName();

    // If there's an existing registrar
    ITypeRegistrar* pExistingType = m_types[typeName];
    assert(pExistingType != pType);     // Sanity check

    // If the new type is the default (built-in) type but it's already been overridden
    if (isDefault && pExistingType)
    {
        return;     // Nothing to do
    }
    // TODO: Inform listeners that there's a new library available
    // and ask if we should use it immediately or defer

    CExchanger exchanger;

    // If the type can be safely created, visited and destroyed
    if (EvaluateType(pType, exchanger))
    {
        // Override the type
        m_types[typeName] = pType;

        if (!isDefault)
        {
            CryLogAlways("SoftCode: Overridden %s in library %s", typeName, m_name);
        }

        const size_t instanceCount = (pExistingType) ? pExistingType->InstanceCount() : 0;

        // If there are any existing instances
        if (instanceCount > 0)
        {
            CryLogAlways("SoftCode:  Attempting to exchange %d %s instances to the new version...", (int)instanceCount, typeName);

            // Read instance members for type (removes data for resolved members)
            if (pExistingType->ExchangeInstances(exchanger))
            {
                exchanger.RewindForLoading();

                // Write instance members for type
                if (pType->ExchangeInstances(exchanger))
                {
                    // Success! Tell the listeners to fix up their pointers
                    for (TListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
                    {
                        exchanger.NotifyListenerOfReplacements(*notifier);
                    }

                    CryLogAlways("SoftCode:  %d %s instances successfully overridden to latest!", (int)instanceCount, typeName);

                    // Clean up old instances
                    if (!pExistingType->DestroyInstances())
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "SoftCode: Failed to destroy old instances of type %s - leak probable.", typeName);
                    }
                }
                else    // Failed to create & write into new instances
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "SoftCode:  Failed to create and write into new instances of %s. Attempting restore of old instances...", typeName);
                    if (!pType->DestroyInstances())
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "SoftCode:  Failed to destroy new instances of type %s - leak probable.", typeName);
                    }

                    // Restore the original type library as the active one
                    m_types[typeName] = pExistingType;

                    // Attempt to restore the old instances with their original data
                    exchanger.RewindForRestore();
                    if (pExistingType->ExchangeInstances(exchanger))
                    {
                        CryLogAlways("SoftCode:  Type %s in library %s successfully restored to previous revision!", typeName, m_name);
                    }
                    else
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "SoftCode:  Restore of old %s instances failed. State now undefined!", typeName);
                    }
                }
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "SoftCode: Failed to read members on %s", typeName);
            }
        }
    }
}

bool DynamicTypeLibrary::EvaluateType(ITypeRegistrar* pType, CExchanger& exchanger)
{
    // Try a full object life-time with a single instance of the
    // new type before attempting a member exchange. This also allow the
    // exchanger to determine the required members to be removed from the
    // old instances.
    bool testPassed = false;

    // Create a single test instance of the type
    if (pType->CreateInstance())
    {
        // Read the instance members (also prepares the exchanger member set)
        if (pType->ExchangeInstances(exchanger))
        {
            // Destroy the old instance
            if (pType->DestroyInstances())
            {
                // Indicate required members are now resolved
                exchanger.LockMemberSet();
                testPassed = true;
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "SoftCode: Failed to destroy test instance of type: %s. New type will be skipped.", pType->GetName());
            }
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "SoftCode: Failed to read members in test instance of type: %s. New type will be skipped.", pType->GetName());
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "SoftCode: Failed to create test instance of type: %s. New type will be skipped.", pType->GetName());
    }

    return testPassed;
}

// ----

// The export we expect to find in the SoftCode modules
typedef ITypeLibrary* (__stdcall * TGetTypeLibraryFcn)();

static void SoftCode_UpdateCmd([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
    gEnv->pSoftCodeMgr->LoadNewModules();
}

static int g_autoUpdatePeriod = 0;

SoftCodeMgr::SoftCodeMgr()
{
    REGISTER_CVAR2("sc_autoupdate", &g_autoUpdatePeriod, 5, VF_CHEAT, "Set the auto-update poll period for new SoftCode modules. Set to zero to disable");
    REGISTER_COMMAND("sc_update", reinterpret_cast<ConsoleCommandFunc>(&SoftCode_UpdateCmd), VF_CHEAT, "Loads any new SoftCode modules");

    // Clear out any old modules
    {
        typedef std::vector<string> TStringVec;
        TStringVec filePaths;

        if (FindSoftCodeFiles("*", filePaths) > 0)
        {
            for (TStringVec::const_iterator iter(filePaths.begin()); iter != filePaths.end(); ++iter)
            {
                if (!DeleteFile(iter->c_str()))
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "SoftCode: Failed to clean %s", iter->c_str());
                }
            }
        }
    }
}

SoftCodeMgr::~SoftCodeMgr()
{
    if (gEnv->pConsole)
    {
        gEnv->pConsole->RemoveCommand("sc_update");
        gEnv->pConsole->UnregisterVariable("sc_autoupdate");
    }
}

// Used to register built-in libraries on first use
void SoftCodeMgr::RegisterLibrary(ITypeLibrary* pDefaultLib)
{
    DynamicTypeLibrary& typeLib = m_libraryMap[pDefaultLib->GetName()];
    typeLib.IntegrateLibrary(pDefaultLib, true);
}

// Look for new SoftCode modules and load them, adding their types to the registry
void SoftCodeMgr::LoadNewModules()
{
    typedef std::vector<string> TStringVec;
    typedef TStringVec::const_iterator TModuleIter;
    TStringVec modulePaths;

    // Find modules
    FindSoftCodeFiles("*.dll", modulePaths);

    for (TModuleIter libIter(modulePaths.begin()); libIter != modulePaths.end(); ++libIter)
    {
        const char* moduleName = libIter->c_str();
        LoadModule(moduleName);
    }
}

void SoftCodeMgr::AddListener(const char* libraryName, ISoftCodeListener* pListener, const char* listenerName)
{
    // Find an existing lib or create a new one to add the listener to
    DynamicTypeLibrary& lib = m_libraryMap[libraryName];
    lib.AddListener(libraryName, pListener, listenerName);
}

void SoftCodeMgr::RemoveListener(const char* libraryName, ISoftCodeListener* pListener)
{
    TLibMap::iterator iter(m_libraryMap.find(libraryName));

    if (iter != m_libraryMap.end())
    {
        iter->second.RemoveListener(pListener);
    }
}

// To be called regularly to poll for library updates
void SoftCodeMgr::PollForNewModules()
{
    if (g_autoUpdatePeriod > 0)
    {
        const CTimeValue frameStartTime(gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI));
        if (m_nextAutoCheckTime <= frameStartTime)
        {
            m_nextAutoCheckTime.SetSeconds((int64)g_autoUpdatePeriod);
            m_nextAutoCheckTime += frameStartTime;

            // Attempt to find and load any new modules
            LoadNewModules();
        }
    }
}

namespace
{
    // Util
    class InstanceFixup
        : public ISoftCodeListener
    {
    public:
        InstanceFixup(void* pOldInstance)
            : m_pOldInstance(pOldInstance)
            , m_pNewInstance() {}

        virtual void InstanceReplaced(void* pOldInstance, void* pNewInstance)
        {
            if (m_pOldInstance == pOldInstance)
            {
                m_pNewInstance = pNewInstance;
            }
        }

        void* NewInstance() const { return m_pNewInstance; }

    private:
        void* m_pOldInstance;
        void* m_pNewInstance;
    };
}

// Stops thread execution until a new SoftCode module is available
void* SoftCodeMgr::WaitForUpdate(void* pInstance)
{
    DynamicTypeLibrary* pOwningLib = NULL;
    ITypeRegistrar* pOldType = NULL;

    // Find existing instance
    for (TLibMap::iterator libIter(m_libraryMap.begin()); libIter != m_libraryMap.end(); ++libIter)
    {
        DynamicTypeLibrary& lib = libIter->second;
        if (ITypeRegistrar* pType = lib.FindTypeForInstance(pInstance))
        {
            pOwningLib = &lib;
            pOldType = pType;
            break;
        }
    }

    if (!pOwningLib)
    {
        CryFatalError("SoftCode: Attempting to wait for update on an unknown instance!");
        return NULL;
    }

    InstanceFixup instanceFixup(pInstance);
    pOwningLib->AddListener(pOwningLib->GetName(), &instanceFixup, "InstanceFixup");

    while (true)
    {
        // Find and load new modules
        LoadNewModules();

        // Got a new instance?
        if (instanceFixup.NewInstance())
        {
            break;
        }

        // Wait for a new module
        CryLogAlways("SoftCode: Pausing execution until class %s in %s library is updated...", pOldType->GetName(), pOwningLib->GetName());
        __debugbreak();     // Stopped here? Check your log!
    }

    pOwningLib->RemoveListener(&instanceFixup);

    return instanceFixup.NewInstance();
}

bool SoftCodeMgr::LoadModule(const char* moduleName)
{
    bool success = false;

    // If module not yet loaded
    if (m_loadedSet.find(moduleName) == m_loadedSet.end())
    {
        m_loadedSet.insert(moduleName);

        CryLogAlways("SoftCode: Found new module %s, attempting to load...", moduleName);

        HMODULE hModule = CryLoadLibrary(moduleName);

        if (hModule)
        {
            TGetTypeLibraryFcn pGetTypeLibraryFcn = reinterpret_cast<TGetTypeLibraryFcn>(GetProcAddress(hModule, DLL_GETTYPELIBRARY));
            if (pGetTypeLibraryFcn)
            {
                // Add to list of loaded libs & override any earlier TypeLibraries already registered
                ITypeLibrary* pTypeLibrary = pGetTypeLibraryFcn();
                if (pTypeLibrary)
                {
                    const char* libraryName = pTypeLibrary->GetName();
                    m_libraryMap[libraryName].IntegrateLibrary(pTypeLibrary, false);

                    CryLogAlways("SoftCode: Loaded new type library \"%s\" from module %s.", libraryName, moduleName);
                    success = true;
                }
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to resolve GetTypeLibrary() export in: %s (error: %x)", moduleName, GetLastError());
            }
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to load: %s (error: %x)", moduleName, GetLastError());
        }
    }

    return success;
}

size_t SoftCodeMgr::FindSoftCodeFiles(const string& searchName, std::vector<string>& foundPaths) const
{
    foundPaths.clear();

    stack_string scSoftCodeDir;

    TCHAR modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, sizeof(modulePath));
    scSoftCodeDir = PathUtil::GetParentDirectory(modulePath);
    scSoftCodeDir += "\\SoftCode\\";


    gEnv->pFileIO->FindFiles(scSoftCodeDir.c_str(), searchName, [&](const char* filePath) -> bool
        {
            if (!gEnv->pFileIO->IsDirectory(filePath) && !gEnv->pFileIO->IsReadOnly(filePath))
            {
                foundPaths.push_back(filePath);
            }

            // Keep asking for more files, no early out
            return true;
        });

    // Sort the paths into name order
    std::sort(foundPaths.begin(), foundPaths.end());

    return foundPaths.size();
}

#endif      // SOFTCODE_ENABLED
