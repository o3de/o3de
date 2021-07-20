/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "WipFeatureManager.h"

#ifdef USE_WIP_FEATURES_MANAGER
#include "WipFeaturesDlg.h"

#if defined(AZ_PLATFORM_WINDOWS)
const char* CWipFeatureManager::kWipFeaturesFilename = "@user@\\Editor\\UI\\WipFeatures.xml";
#else
const char* CWipFeatureManager::kWipFeaturesFilename = "@user@/Editor/UI/WipFeatures.xml";
#endif
CWipFeatureManager* CWipFeatureManager::s_pInstance = NULL;

static void WipFeatureVarChange(ICVar* pVar)
{
    QString strParams = pVar->GetString();
    QStringList params;

    SplitString(strParams, params, ' ');

    if (strParams == "edit")
    {
        static CWipFeaturesDlg dlg;

        dlg.show();

        return;
    }

    if (params.size() >= 2)
    {
        QString featName = params[0].trimmed();
        QString attr = params[1].trimmed();

        if (featName.isEmpty())
        {
            return;
        }

        int id = featName.toInt();

        // if all features
        if (featName == "*")
        {
            if (attr == "enable")
            {
                CWipFeatureManager::Instance()->EnableAllFeatures(true);
            }
            else
            if (attr == "disable")
            {
                CWipFeatureManager::Instance()->EnableAllFeatures(false);
            }
            else
            if (attr == "hide")
            {
                CWipFeatureManager::Instance()->ShowAllFeatures(false);
            }
            else
            if (attr == "show")
            {
                CWipFeatureManager::Instance()->ShowAllFeatures(true);
            }
            else
            if (attr == "safemode")
            {
                CWipFeatureManager::Instance()->SetAllFeaturesSafeMode(true);
            }
            else
            if (attr == "fullmode")
            {
                CWipFeatureManager::Instance()->SetAllFeaturesSafeMode(false);
            }
            else
            {
                CWipFeatureManager::Instance()->SetAllFeaturesParams(attr.toUtf8().data());
            }

            return;
        }

        if (attr == "enable")
        {
            CWipFeatureManager::Instance()->EnableFeature(id, true);
        }
        else
        if (attr == "disable")
        {
            CWipFeatureManager::Instance()->EnableFeature(id, false);
        }
        else
        if (attr == "hide")
        {
            CWipFeatureManager::Instance()->ShowFeature(id, false);
        }
        else
        if (attr == "show")
        {
            CWipFeatureManager::Instance()->ShowFeature(id, true);
        }
        else
        if (attr == "safemode")
        {
            CWipFeatureManager::Instance()->SetFeatureSafeMode(id, true);
        }
        else
        if (attr == "fullmode")
        {
            CWipFeatureManager::Instance()->SetFeatureSafeMode(id, false);
        }
        else
        {
            CWipFeatureManager::Instance()->SetFeatureParams(id, attr.toUtf8().data());
        }
    }
}

CWipFeatureManager::CWipFeatureManager()
{
    m_bEnabled = true;
}

CWipFeatureManager::~CWipFeatureManager()
{
}

bool CWipFeatureManager::Init(bool bLoadXml)
{
    if (!gEnv)
    {
        return false;
    }

    IConsole* pConsole = gEnv->pConsole;

    if (!pConsole)
    {
        return false;
    }

    REGISTER_CVAR2_CB("e_wipfeature", (const char**)&CWipFeatureManager::Instance()->m_consoleCmdParams, "", VF_ALWAYSONCHANGE | VF_CHEAT, "wipfeature <featureName> enable|disable|hide|show|safemode|fullmode", WipFeatureVarChange);

    if (bLoadXml)
    {
        CWipFeatureManager::Instance()->Load();
    }

    return true;
}

void CWipFeatureManager::Shutdown()
{
    CWipFeatureManager::Instance()->Save();
    delete s_pInstance;
    s_pInstance = NULL;
}

bool CWipFeatureManager::Load(const char* pFilename, bool bClearExisting)
{
    if (!GetISystem())
    {
        return false;
    }

    XmlNodeRef root = GetISystem()->LoadXmlFromFile(pFilename);

    if (!root)
    {
        return false;
    }

    if (bClearExisting)
    {
        m_features.clear();
    }

    Log("Loading WIP features file: '%s'...", pFilename);

    for (size_t i = 0, iCount = root->getChildCount(); i < iCount; ++i)
    {
        SWipFeatureInfo wf;
        XmlNodeRef node = root->getChild(i);
        XmlString str;

        node->getAttr("id", wf.m_id);
        node->getAttr("displayName", str);
        wf.m_displayName = str;
        node->getAttr("visible", wf.m_bVisible);
        node->getAttr("enabled", wf.m_bEnabled);
        node->getAttr("safeMode", wf.m_bSafeMode);
        node->getAttr("params", str);
        wf.m_params = str;
        wf.m_bLoadedFromXml = true;

        TWipFeatures::iterator iter = m_features.find(wf.m_id);

        if (iter == m_features.end())
        {
            m_features[wf.m_id] = wf;
        }
        else
        {
            m_features[wf.m_id].m_bVisible = wf.m_bVisible;
            m_features[wf.m_id].m_bEnabled = wf.m_bEnabled;
            m_features[wf.m_id].m_bSafeMode = wf.m_bSafeMode;
            m_features[wf.m_id].m_params = wf.m_params;
        }
    }

    Log("Loaded %d WIP features.", m_features.size());

    return true;
}

bool CWipFeatureManager::Save(const char* pFilename)
{
    if (!gEnv)
    {
        return false;
    }

    if (!GetISystem())
    {
        return false;
    }

    ISystem* pISystem = GetISystem();

    XmlNodeRef root = pISystem->CreateXmlNode("features");

    for (TWipFeatures::iterator iter = m_features.begin(), iterEnd = m_features.end(); iter != iterEnd; ++iter)
    {
        SWipFeatureInfo& wf = iter->second;
        XmlNodeRef node = root->createNode("feature");

        node->setAttr("id", wf.m_id);
        node->setAttr("displayName", wf.m_displayName.c_str());
        node->setAttr("visible", wf.m_bVisible);
        node->setAttr("enabled", wf.m_bEnabled);
        node->setAttr("safeMode", wf.m_bSafeMode);
        node->setAttr("params", wf.m_params.c_str());

        root->addChild(node);
    }

    root->saveToFile(pFilename);

    return true;
}

int CWipFeatureManager::RegisterFeature(const char* pDisplayName, bool bVisible, bool bEnabled, bool bSafeMode, const char* pParams, bool bSaveToXml)
{
    int aMaxId = -1;

    for (TWipFeatures::iterator iter = m_features.begin(), iterEnd = m_features.end(); iter != iterEnd; ++iter)
    {
        if (iter->first > aMaxId)
        {
            aMaxId = iter->first;
        }
    }

    ++aMaxId;
    SetFeature(aMaxId, pDisplayName, bVisible, bEnabled, bSafeMode, pParams, bSaveToXml);

    return aMaxId;
}

void CWipFeatureManager::SetFeature(int aFeatureId, const char* pDisplayName, bool bVisible, bool bEnabled, bool bSafeMode, const char* pParams, bool bSaveToXml)
{
    m_features[aFeatureId].m_id = aFeatureId;
    m_features[aFeatureId].m_displayName = pDisplayName;
    m_features[aFeatureId].m_bVisible = bVisible;
    m_features[aFeatureId].m_bEnabled = bEnabled;
    m_features[aFeatureId].m_bSafeMode = bSafeMode;
    m_features[aFeatureId].m_bSaveToXml = bSaveToXml;
    m_features[aFeatureId].m_params = pParams;

    if (m_features[aFeatureId].m_pfnUpdateFeature)
    {
        m_features[aFeatureId].m_pfnUpdateFeature(aFeatureId, &bVisible, &bEnabled, &bSafeMode, pParams);
    }
}

void CWipFeatureManager::SetDefaultFeatureStates(int aFeatureId, const char* pDisplayName, bool bVisible, bool bEnabled, bool bSafeMode, const char* pParams)
{
    TWipFeatures::iterator iter = m_features.find(aFeatureId);

    // set feature if not existing
    if (iter == m_features.end() || (iter != m_features.end() && !iter->second.m_bLoadedFromXml))
    {
        m_features[aFeatureId].m_id = aFeatureId;
        m_features[aFeatureId].m_displayName = pDisplayName;
        m_features[aFeatureId].m_bVisible = bVisible;
        m_features[aFeatureId].m_bEnabled = bEnabled;
        m_features[aFeatureId].m_bSafeMode = bSafeMode;
        m_features[aFeatureId].m_params = pParams;
    }
    else
    if (iter != m_features.end() && iter->second.m_bLoadedFromXml)
    {
        m_features[aFeatureId].m_id = aFeatureId;
        m_features[aFeatureId].m_displayName = pDisplayName;
    }

    if (m_features[aFeatureId].m_pfnUpdateFeature)
    {
        m_features[aFeatureId].m_pfnUpdateFeature(aFeatureId, &bVisible, &bEnabled, &bSafeMode, pParams);
    }
}

bool CWipFeatureManager::IsFeatureVisible(int aFeatureId)
{
    return m_features[aFeatureId].m_bVisible || !m_bEnabled;
}

bool CWipFeatureManager::IsFeatureEnabled(int aFeatureId)
{
    return m_features[aFeatureId].m_bEnabled || !m_bEnabled;
}

bool CWipFeatureManager::IsFeatureInSafeMode(int aFeatureId)
{
    if (!m_bEnabled)
    {
        return false;
    }

    return m_features[aFeatureId].m_bSafeMode;
}

const char* CWipFeatureManager::GetFeatureParams(int aFeatureId)
{
    return m_features[aFeatureId].m_params.c_str();
}

void CWipFeatureManager::ShowFeature(int aFeatureId, bool bShow)
{
    m_features[aFeatureId].m_bVisible = bShow;

    if (m_features[aFeatureId].m_pfnUpdateFeature)
    {
        m_features[aFeatureId].m_pfnUpdateFeature(aFeatureId, &bShow, NULL, NULL, NULL);
    }
}

void CWipFeatureManager::EnableFeature(int aFeatureId, bool bEnable)
{
    m_features[aFeatureId].m_bEnabled = bEnable;

    if (m_features[aFeatureId].m_pfnUpdateFeature)
    {
        m_features[aFeatureId].m_pfnUpdateFeature(aFeatureId, NULL, &bEnable, NULL, NULL);
    }
}

void CWipFeatureManager::SetFeatureSafeMode(int aFeatureId, bool bSafeMode)
{
    m_features[aFeatureId].m_bSafeMode = bSafeMode;

    if (m_features[aFeatureId].m_pfnUpdateFeature)
    {
        m_features[aFeatureId].m_pfnUpdateFeature(aFeatureId, NULL, NULL, &bSafeMode, NULL);
    }
}

void CWipFeatureManager::SetFeatureParams(int aFeatureId, const char* pParams)
{
    m_features[aFeatureId].m_params = pParams;

    if (m_features[aFeatureId].m_pfnUpdateFeature)
    {
        m_features[aFeatureId].m_pfnUpdateFeature(aFeatureId, NULL, NULL, NULL, pParams);
    }
}

void CWipFeatureManager::ShowAllFeatures(bool bShow)
{
    for (TWipFeatures::iterator iter = m_features.begin(), iterEnd = m_features.end(); iter != iterEnd; ++iter)
    {
        iter->second.m_bVisible = bShow;

        if (iter->second.m_pfnUpdateFeature)
        {
            iter->second.m_pfnUpdateFeature(iter->first, &bShow, NULL, NULL, NULL);
        }
    }
}

void CWipFeatureManager::EnableAllFeatures(bool bEnable)
{
    for (TWipFeatures::iterator iter = m_features.begin(), iterEnd = m_features.end(); iter != iterEnd; ++iter)
    {
        iter->second.m_bEnabled = bEnable;

        if (iter->second.m_pfnUpdateFeature)
        {
            iter->second.m_pfnUpdateFeature(iter->first, NULL, &bEnable, NULL, NULL);
        }
    }
}

void CWipFeatureManager::SetAllFeaturesSafeMode(bool bSafeMode)
{
    for (TWipFeatures::iterator iter = m_features.begin(), iterEnd = m_features.end(); iter != iterEnd; ++iter)
    {
        iter->second.m_bSafeMode = bSafeMode;

        if (iter->second.m_pfnUpdateFeature)
        {
            iter->second.m_pfnUpdateFeature(iter->first, NULL, NULL, &bSafeMode, NULL);
        }
    }
}

void CWipFeatureManager::SetAllFeaturesParams(const char* pParams)
{
    for (TWipFeatures::iterator iter = m_features.begin(), iterEnd = m_features.end(); iter != iterEnd; ++iter)
    {
        iter->second.m_params = pParams;

        if (iter->second.m_pfnUpdateFeature)
        {
            iter->second.m_pfnUpdateFeature(iter->first, NULL, NULL, NULL, pParams);
        }
    }
}

void CWipFeatureManager::EnableManager(bool bEnable)
{
    m_bEnabled = bEnable;
}

void CWipFeatureManager::SetFeatureUpdateCallback(int aFeatureId, TWipFeatureUpdateCallback pfnUpdate)
{
    m_features[aFeatureId].m_pfnUpdateFeature = pfnUpdate;
}

std::map<int, CWipFeatureManager::SWipFeatureInfo>&  CWipFeatureManager::GetFeatures()
{
    return m_features;
}

#endif
