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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_OBJECTPHYSICSMANAGER_H
#define CRYINCLUDE_EDITOR_OBJECTS_OBJECTPHYSICSMANAGER_H
#pragma once


//////////////////////////////////////////////////////////////////////////
class CObjectPhysicsManager
{
public:
    CObjectPhysicsManager();
    ~CObjectPhysicsManager();

    void SimulateSelectedObjectsPositions();
    void Update();

    //////////////////////////////////////////////////////////////////////////
    /// Collision Classes
    //////////////////////////////////////////////////////////////////////////
    int RegisterCollisionClass(const SCollisionClass& collclass);
    int GetCollisionClassId(const SCollisionClass& collclass);
    void SerializeCollisionClasses(CXmlArchive& xmlAr);

    void PrepareForExport();

private:
    void Command_SimulateObjects();
    void Command_GetPhysicsState();
    void Command_ResetPhysicsState();

    void UpdateSimulatingObjects();

    bool m_bSimulatingObjects;
    float m_fStartObjectSimulationTime;
    int m_wasSimObjects;
    std::vector<_smart_ptr<CBaseObject> > m_simObjects;

    typedef std::vector<SCollisionClass> TCollisionClassVector;
    int m_collisionClassExportId;
    TCollisionClassVector m_collisionClasses;
};


#endif // CRYINCLUDE_EDITOR_OBJECTS_OBJECTPHYSICSMANAGER_H
