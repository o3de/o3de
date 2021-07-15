/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_WIPFEATUREMANAGER_H
#define CRYINCLUDE_EDITOR_WIPFEATUREMANAGER_H
#pragma once

/*
    This class is used to control work in progress features at runtime, so QA can test even if the end user will not see those features
    You can use the console command: e_wipfeature <numeric featureId> enable|disable|hide|show|safemode|fullmode
    ******************************************************************************************************************************************
    *** GOOD TO KNOW: "e_wipfeature edit" console command will display the WIP dialog and you can control the features from there
    ******************************************************************************************************************************************
*/

// undef this define to spot all wip feature usages within the editor at compile time
#define USE_WIP_FEATURES_MANAGER

#ifdef USE_WIP_FEATURES_MANAGER

// use this to register new wip features, usage from inside functions
// @param id is the numeric unique id of the feature, its good to have all feature ids in an enum in one file
// @param bVisible is the feature visible by default
// @param bEnabled is the feature enabled (usually visual enable like non-grayed) by default
// @param bSafeMode is the feature operating in some sort of safe mode (the safe mode behavior defined by the feature itself)
// @param pTWipFeatureUpdateCallback callback of type TWipFeatureUpdateCallback for when a feature state (visible,enabled and so on) was modified
#define REGISTER_WIP_FEATURE(id, bVisible, bEnabled, bSafeMode, pTWipFeatureUpdateCallback) \
    static CWipFeatureManager::CWipFeatureRegisterer s_wipFeatureRegisterer_##id(id, ""#id, bVisible, bEnabled, bSafeMode, pTWipFeatureUpdateCallback);

#define IS_WIP_FEATURE_VISIBLE(id) CWipFeatureManager::Instance()->IsFeatureVisible(id)
#define IS_WIP_FEATURE_ENABLED(id) CWipFeatureManager::Instance()->IsFeatureEnabled(id)
#define IS_WIP_FEATURE_SAFEMODE(id) CWipFeatureManager::Instance()->IsFeatureInSafeMode(id)

// The feature manager singleton itself
class CWipFeatureManager
{
public:

    static const char* kWipFeaturesFilename;

    // Used to register a callback function to update the state of features whitin the editor
    // pbVisible, pbEnabled, pbSafeMode, pParams - if the pointer is NULL, then that attribute was not changed
    typedef void (* TWipFeatureUpdateCallback)(int aFeatureId, const bool* const pbVisible, const bool* const pbEnabled, const bool* const pbSafeMode, const char* pParams);

    // wip feature registerer auto create object, used for static auto feature creation with the REGISTER_WIP_FEATURE macro
    class CWipFeatureRegisterer
    {
    public:

        CWipFeatureRegisterer(int id, const char* pDisplayName, bool bVisible, bool bEnabled, bool bSafeMode, TWipFeatureUpdateCallback pTWipFeatureUpdateCallback)
        {
            CWipFeatureManager::Instance()->SetFeatureUpdateCallback(id, pTWipFeatureUpdateCallback);
            CWipFeatureManager::Instance()->SetDefaultFeatureStates(id, pDisplayName, bVisible, bEnabled, bSafeMode);
        }
    };

    struct SWipFeatureInfo
    {
        SWipFeatureInfo()
            : m_id(0)
            , m_displayName("")
            , m_bVisible(true)
            , m_bEnabled(true)
            , m_bSafeMode(false)
            , m_pfnUpdateFeature(NULL)
            , m_bLoadedFromXml(false)
        {}

        int         m_id;
        string  m_displayName, m_params;
        bool        m_bVisible, m_bEnabled, m_bSafeMode,
        // if true, this feature will be saved into the xml file when Save(...) will be called
                    m_bSaveToXml, m_bLoadedFromXml;
        TWipFeatureUpdateCallback   m_pfnUpdateFeature;
    };

    typedef std::map<int, SWipFeatureInfo> TWipFeatures;

private:

    CWipFeatureManager();
    ~CWipFeatureManager();

    static CWipFeatureManager* s_pInstance;

public:

    static CWipFeatureManager* Instance()
    {
        if (!s_pInstance)
        {
            s_pInstance = new CWipFeatureManager();
            assert(s_pInstance);
        }

        return s_pInstance;
    }

    static bool Init(bool bLoadXml = true);
    static void Shutdown();

    bool Load(const char* pFilename = kWipFeaturesFilename, bool bClearExisting = true);
    bool Save(const char* pFilename = kWipFeaturesFilename);

    // Register a new feature
    // @return a new feature ID
    int     RegisterFeature(const char* pDisplayName, bool bVisible, bool bEnabled, bool bSafeMode, const char* pParams = "", bool bSaveToXml = true);
    // Set an existing feature
    void    SetFeature(int aFeatureId, const char* pDisplayName, bool bVisible, bool bEnabled, bool bSafeMode, const char* pParams = "", bool bSaveToXml = true);
    // Create a new feature, but it will take into account the existing feature info from the loaded XML file, with persistent settings, if any
    void    SetDefaultFeatureStates(int aFeatureId, const char* pDisplayName, bool bVisible, bool bEnabled, bool bSafeMode, const char* pParams = "");
    bool    IsFeatureVisible(int aFeatureId);
    bool    IsFeatureEnabled(int aFeatureId);
    bool    IsFeatureInSafeMode(int aFeatureId);
    const char* GetFeatureParams(int aFeatureId);

    void ShowFeature(int aFeatureId, bool bShow = true);
    void EnableFeature(int aFeatureId, bool bEnable = true);
    void SetFeatureSafeMode(int aFeatureId, bool bSafeMode);
    void SetFeatureParams(int aFeatureId, const char* pParams);

    void ShowAllFeatures(bool bShow = true);
    void EnableAllFeatures(bool bEnable = true);
    void SetAllFeaturesSafeMode(bool bSafeMode);
    void SetAllFeaturesParams(const char* pParams);

    // if manager is disabled, then all queries about feature enable/visible/fullmode states will return true always
    void EnableManager(bool bEnable = true);

    void SetFeatureUpdateCallback(int aFeatureId, TWipFeatureUpdateCallback pfnUpdate);
    TWipFeatures&   GetFeatures();

private:

    static const int kMaxWipCmdSize = 200;

    TWipFeatures    m_features;
    char                    m_consoleCmdParams[kMaxWipCmdSize];
    bool                    m_bEnabled;
};

#else

//
// no WIP feature manager in production build
//
#define REGISTER_WIP_FEATURE
#define IS_WIP_FEATURE_VISIBLE(id) true
#define IS_WIP_FEATURE_ENABLED(id) true
#define IS_WIP_FEATURE_SAFEMODE(id) true

#endif //USE_WIP_FEATURES_MANAGER
#endif // CRYINCLUDE_EDITOR_WIPFEATUREMANAGER_H
