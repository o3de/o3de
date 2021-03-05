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

// Description : Mission class definition.

#pragma once


struct SMinimapInfo
{
    Vec2 vCenter;
    Vec2 vExtends;
    //  float RenderBoxSize;
    int textureWidth;
    int textureHeight;
    int orientation;
};

/*!
        CMission represent single Game Mission on same map.
        Multiple Missions share same map, and stored in one .cry or .ly file.

 */
class CMission
{
public:
    //! Ctor of mission.
    CMission(CCryEditDoc* doc);
    //! Dtor of mission.
    virtual ~CMission();

    void SetName(const QString& name) { m_name = name; }
    const QString& GetName() const { return m_name; }

    void SetDescription(const QString& dsc) { m_description = dsc; }
    const QString& GetDescription() const { return m_description; }

    XmlNodeRef GetEnvironment() { return m_environment; };

    void SetTime(float time) { m_time = time; };
    float GetTime() const { return m_time; };

    //! Called when this mission must be synchonized with current data in Document.
    //! if bRetrieve is true, data is retrieved from Mission to global structures.
    void    SyncContent(bool bRetrieve, bool bIgnoreObjects, bool bSkipLoadingAI = false);

    //! Create clone of this mission.
    CMission*   Clone();

    //! Serialize mission.
    void Serialize(CXmlArchive& ar, bool bParts = true);

    //! Serialize time of day
    void SerializeTimeOfDay(CXmlArchive& ar);

    //! Serialize environment
    void SerializeEnvironment(CXmlArchive& ar);

    //! Save some elements of mission to separate files
    void SaveParts();

    //! Load some elements of mission from separate files
    void LoadParts();

    //! Export mission to game.
    void Export(XmlNodeRef& root, XmlNodeRef& objectsNode);

    //! Add shared objects to mission objects.
    void AddObjectsNode(XmlNodeRef& node);
    void SetLayersNode(XmlNodeRef& node);

    void OnEnvironmentChange();
    int GetNumCGFObjects() const { return m_numCGFObjects; };

    //////////////////////////////////////////////////////////////////////////
    // Minimap.
    void SetMinimap(const SMinimapInfo& info);
    const SMinimapInfo& GetMinimap() const;

private:
    //! Document owner of this mission.
    CCryEditDoc* m_doc;

    QString m_name;
    QString m_description;

    //! Mission time;
    float   m_time;

    //! Root node of objects defined only in this mission.
    XmlNodeRef m_objects;

    //! Object layers.
    XmlNodeRef m_layers;

    //! Exported data of this mission.
    XmlNodeRef m_exportData;

    //! Environment settings of this mission.
    XmlNodeRef m_environment;

    XmlNodeRef m_Animations; // backward compatibility.

    XmlNodeRef m_timeOfDay;

    int m_numCGFObjects;

    SMinimapInfo m_minimap;

    bool m_reentrancyProtector;
};

