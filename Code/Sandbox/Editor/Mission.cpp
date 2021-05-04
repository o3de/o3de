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

// Description : CMission class implementation.

#include "EditorDefs.h"

#include "Mission.h"

// cryCommon
#include <CryCommon/ITimeOfDay.h>
#include <CryCommon/I3DEngine.h>

// Editor
#include "CryEditDoc.h"
#include "GameEngine.h"
#include "Include/IObjectManager.h"


namespace
{
    const char* kTimeOfDayFile = "TimeOfDay.xml";
    const char* kTimeOfDayRoot = "TimeOfDay";
    const char* kEnvironmentFile = "Environment.xml";
    const char* kEnvironmentRoot = "Environment";
};


//////////////////////////////////////////////////////////////////////////
CMission::CMission(CCryEditDoc* doc)
{
    m_doc = doc;
    m_objects = XmlHelpers::CreateXmlNode("Objects");
    m_layers = XmlHelpers::CreateXmlNode("ObjectLayers");
    //m_exportData = XmlNodeRef( "ExportData" );
    m_timeOfDay = XmlHelpers::CreateXmlNode("TimeOfDay");
    m_environment = XmlHelpers::CreateXmlNode("Environment");
    CXmlTemplate::SetValues(m_doc->GetEnvironmentTemplate(), m_environment);

    m_time = 12; // 12 PM by default.

    m_numCGFObjects = 0;

    m_reentrancyProtector = false;
}

//////////////////////////////////////////////////////////////////////////
CMission::~CMission()
{
}

//////////////////////////////////////////////////////////////////////////
CMission* CMission::Clone()
{
    CMission* m = new CMission(m_doc);
    m->SetName(m_name);
    m->SetDescription(m_description);
    m->m_objects = m_objects->clone();
    m->m_layers = m_layers->clone();
    m->m_environment = m_environment->clone();
    m->m_time = m_time;
    return m;
}

//////////////////////////////////////////////////////////////////////////
void CMission::Serialize(CXmlArchive& ar, bool bParts)
{
    if (ar.bLoading)
    {
        // Load.
        ar.root->getAttr("Name", m_name);
        ar.root->getAttr("Description", m_description);

        XmlNodeRef objects = ar.root->findChild("Objects");
        if (objects)
        {
            m_objects = objects;
        }

        XmlNodeRef layers = ar.root->findChild("ObjectLayers");
        if (layers)
        {
            m_layers = layers;
        }

        SerializeTimeOfDay(ar);

        m_Animations = ar.root->findChild("MovieData");

        SerializeEnvironment(ar);
    }
    else
    {
        ar.root->setAttr("Name", m_name.toUtf8().data());
        ar.root->setAttr("Description", m_description.toUtf8().data());

        QString timeStr;
        int nHour = floor(m_time);
        int nMins = (m_time - floor(m_time)) * 60.0f;
        timeStr = QStringLiteral("%1:%2").arg(nHour, 2, 10, QLatin1Char('0')).arg(nMins, 2, 10, QLatin1Char('0'));
        ar.root->setAttr("MissionTime", timeStr.toUtf8().data());

        // Saving.
        XmlNodeRef layers = m_layers->clone();
        layers->setTag("ObjectLayers");
        ar.root->addChild(layers);

        ///XmlNodeRef objects = m_objects->clone();
        m_objects->setTag("Objects");
        ar.root->addChild(m_objects);

        if (bParts)
        {
            SerializeTimeOfDay(ar);
            SerializeEnvironment(ar);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::Export(XmlNodeRef& root, XmlNodeRef& objectsNode)
{
    // Also save exported objects data.
    root->setAttr("Name", m_name.toUtf8().data());
    root->setAttr("Description", m_description.toUtf8().data());

    QString timeStr;
    int nHour = floor(m_time);
    int nMins = (m_time - floor(m_time)) * 60.0f;
    timeStr = QStringLiteral("%1:%2").arg(nHour, 2, 10, QLatin1Char('0')).arg(nMins, 2, 10, QLatin1Char('0'));
    root->setAttr("Time", timeStr.toUtf8().data());

    // Saving.
    //XmlNodeRef objects = m_exportData->clone();
    //objects->setTag( "Objects" );
    //root->addChild( objects );

    XmlNodeRef envNode = m_environment->clone();
    root->addChild(envNode);

    m_timeOfDay->setAttr("Time", m_time);
    root->addChild(m_timeOfDay);

    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();

    //////////////////////////////////////////////////////////////////////////
    // Serialize objects.
    //////////////////////////////////////////////////////////////////////////
    QString path = QDir::toNativeSeparators(QFileInfo(m_doc->GetLevelPathName()).absolutePath());
    if (!path.endsWith(QDir::separator()))
        path += QDir::separator();

    objectsNode = root->newChild("Objects");
    pObjMan->Export(path, objectsNode, true); // Export shared.
    pObjMan->Export(path, objectsNode, false);  // Export not shared.
}

//////////////////////////////////////////////////////////////////////////
void CMission::SyncContent(bool bRetrieve, bool bIgnoreObjects, [[maybe_unused]] bool bSkipLoadingAI /* = false */)
{
    // The function may take a longer time when executing objMan->Serialize, which uses CWaitProgress internally
    // Adding a sync flag to prevent the function from being re-entered after the data is modified by OnEnvironmentChange
    if (m_reentrancyProtector)
    {
        return;
    }
    m_reentrancyProtector = true;

    // Save data from current Document to Mission.
    IObjectManager* objMan = GetIEditor()->GetObjectManager();
    if (bRetrieve)
    {
        // Activating this mission.
        CGameEngine* gameEngine = GetIEditor()->GetGameEngine();

        if (!bIgnoreObjects)
        {
            // Retrieve data from Mission and put to document.
            XmlNodeRef root = XmlHelpers::CreateXmlNode("Root");
            root->addChild(m_objects);
            root->addChild(m_layers);
            objMan->Serialize(root, true, SERIALIZE_ONLY_NOTSHARED);
        }

        m_doc->GetFogTemplate() = m_environment;

        CXmlTemplate::GetValues(m_doc->GetEnvironmentTemplate(), m_environment);

        gameEngine->ReloadEnvironment();

        objMan->SendEvent(EVENT_MISSION_CHANGE);
        m_doc->ChangeMission();

        if (GetIEditor()->Get3DEngine())
        {
            m_numCGFObjects = GetIEditor()->Get3DEngine()->GetLoadedObjectCount();

            // Load time of day.
            GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_timeOfDay, true);
        }
    }
    else
    {
        // Save time of day.
        if (GetIEditor()->Get3DEngine())
        {
            m_timeOfDay = XmlHelpers::CreateXmlNode("TimeOfDay");
            GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize(m_timeOfDay, false);
        }

        if (!bIgnoreObjects)
        {
            XmlNodeRef root = XmlHelpers::CreateXmlNode("Root");
            objMan->Serialize(root, false, SERIALIZE_ONLY_NOTSHARED);
            m_objects = root->findChild("Objects");
            XmlNodeRef layers = root->findChild("ObjectLayers");
            if (layers)
            {
                m_layers = layers;
            }
        }
    }

    m_reentrancyProtector = false;
}

//////////////////////////////////////////////////////////////////////////
void CMission::OnEnvironmentChange()
{
    // Only execute the reload function if there is no ongoing SyncContent.
    if (m_reentrancyProtector)
    {
        return;
    }
    m_reentrancyProtector = true;
    m_environment = XmlHelpers::CreateXmlNode("Environment");
    CXmlTemplate::SetValues(m_doc->GetEnvironmentTemplate(), m_environment);
    m_reentrancyProtector = false;
}

//////////////////////////////////////////////////////////////////////////
void CMission::AddObjectsNode(XmlNodeRef& node)
{
    for (int i = 0; i < node->getChildCount(); i++)
    {
        m_objects->addChild(node->getChild(i)->clone());
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::SetLayersNode(XmlNodeRef& node)
{
    m_layers = node->clone();
}

//////////////////////////////////////////////////////////////////////////
void CMission::SaveParts()
{
    // Save Time of Day
    {
        CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kTimeOfDayFile).toUtf8().data());

        m_timeOfDay->saveToFile(helper.GetTempFilePath().toUtf8().data());

        if (!helper.UpdateFile(false))
        {
            return;
        }
    }


    // Save Environment
    {
        CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kEnvironmentFile).toUtf8().data());

        XmlNodeRef root = m_environment->clone();
        root->setTag(kEnvironmentRoot);
        root->saveToFile(helper.GetTempFilePath().toUtf8().data());

        if (!helper.UpdateFile(false))
        {
            return;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CMission::LoadParts()
{
    // Load Time of Day
    {
        QString filename = GetIEditor()->GetLevelDataFolder() + kTimeOfDayFile;
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
        if (root && !_stricmp(root->getTag(), kTimeOfDayRoot))
        {
            m_timeOfDay = root;
            m_timeOfDay->getAttr("Time", m_time);
        }
    }

    // Load Environment
    {
        QString filename = GetIEditor()->GetLevelDataFolder() + kEnvironmentFile;
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
        if (root && !_stricmp(root->getTag(), kEnvironmentRoot))
        {
            m_environment = root;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::SerializeTimeOfDay(CXmlArchive& ar)
{
    if (ar.bLoading)
    {
        XmlNodeRef todNode = ar.root->findChild("TimeOfDay");
        if (todNode)
        {
            m_timeOfDay = todNode;
            todNode->getAttr("Time", m_time);
        }
        else
        {
            m_timeOfDay = XmlHelpers::CreateXmlNode("TimeOfDay");
        }
    }
    else
    {
        m_timeOfDay->setAttr("Time", m_time);
        ar.root->addChild(m_timeOfDay);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMission::SerializeEnvironment(CXmlArchive& ar)
{
    if (ar.bLoading)
    {
        XmlNodeRef env = ar.root->findChild("Environment");
        if (env)
        {
            m_environment = env;
        }
    }
    else
    {
        XmlNodeRef env = m_environment->clone();
        env->setTag("Environment");
        ar.root->addChild(env);
    }
}

