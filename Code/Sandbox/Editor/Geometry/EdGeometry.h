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

#ifndef CRYINCLUDE_EDITOR_GEOMETRY_EDGEOMETRY_H
#define CRYINCLUDE_EDITOR_GEOMETRY_EDGEOMETRY_H
#pragma once

struct IIndexedMesh;
struct DisplayContext;
struct HitContext;
struct SSubObjSelectionModifyContext;
class CObjectArchive;

// Basic supported geometry types.
enum EEdGeometryType
{
    GEOM_TYPE_MESH = 0, // Mesh geometry.
    GEOM_TYPE_BRUSH,    // Solid brush geometry.
    GEOM_TYPE_PATCH,    // Bezier patch surface geometry.
    GEOM_TYPE_NURB,         // Nurbs surface geometry.
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    CEdGeometry is a base class for all supported editable geometries.
//////////////////////////////////////////////////////////////////////////
class CRYEDIT_API CEdGeometry
    : public CRefCountBase
{
public:
    CEdGeometry() {};

    // Query the type of the geometry mesh.
    virtual EEdGeometryType GetType() const = 0;

    // Serialize geometry.
    virtual void Serialize(CObjectArchive& ar) = 0;

    // Return geometry axis aligned bounding box.
    virtual void GetBounds(AABB& box) = 0;

    // Clones Geometry, returns exact copy of the original geometry.
    virtual CEdGeometry* Clone() = 0;

    // Access to the indexed mesh.
    // Return false if geometry can not be represented by an indexed mesh.
    virtual IIndexedMesh* GetIndexedMesh(size_t idx = 0) = 0;
    virtual IStatObj* GetIStatObj() const = 0;
    virtual void GetTM(Matrix34* pTM, size_t idx = 0) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Advanced geometry interface for SubObject selection and modification.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetModified(bool bModified = true) = 0;
    virtual bool IsModified() const = 0;
    virtual bool StartSubObjSelection(const Matrix34& nodeWorldTM, int elemType, int nFlags) = 0;
    virtual void EndSubObjSelection() = 0;

    // Display geometry for sub object selection.
    virtual void Display(DisplayContext& dc) = 0;

    // Sub geometry hit testing and selection.
    virtual bool HitTest(HitContext& hit) = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void ModifySelection(SSubObjSelectionModifyContext& modCtx, bool isUndo = true) = 0;
    // Called when selection modification is accepted.
    virtual void AcceptModifySelection() = 0;

protected:
    ~CEdGeometry() {};
};

#endif // CRYINCLUDE_EDITOR_GEOMETRY_EDGEOMETRY_H
