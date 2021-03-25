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

#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   ModelViewport.h
//  Version:     v1.00
//  Created:     8/10/2001 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_MODELVIEWPORT_H
#define CRYINCLUDE_EDITOR_MODELVIEWPORT_H

#if !defined(Q_MOC_RUN)
#include "RenderViewport.h"
#include "Material/Material.h"
#include "Util/Variable.h"
#endif

struct IPhysicalEntity;

/////////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
// CModelViewport window
class SANDBOX_API CModelViewport
    : public CRenderViewport
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT
    // Construction
public:
    CModelViewport(const char* settingsPath = "Settings\\CharacterEditorUserOptions", QWidget* parent = nullptr);
    virtual ~CModelViewport();

    virtual EViewportType GetType() const { return ET_ViewportModel; }
    virtual void SetType([[maybe_unused]] EViewportType type) { assert(type == ET_ViewportModel); };

    virtual void LoadObject(const QString& obj, float scale);

    virtual void OnActivate();
    virtual void OnDeactivate();

    virtual bool CanDrop(const QPoint& point, IDataBaseItem* pItem);
    virtual void Drop(const QPoint& point, IDataBaseItem* pItem);

    virtual void SetSelected(bool const bSelect);

    // Callbacks.
    void OnShowShaders(IVariable* var);
    void OnShowNormals(IVariable* var);
    void OnShowTangents(IVariable* var);

    void OnShowPortals(IVariable* var);
    void OnShowShadowVolumes(IVariable* var);
    void OnShowTextureUsage(IVariable* var);
    void OnCharPhysics(IVariable* var);
    void OnShowOcclusion(IVariable* var);

    void OnLightColor(IVariable* var);
    void OnLightMultiplier(IVariable* var);
    void OnDisableVisibility(IVariable* var);

    IStatObj* GetStaticObject(){ return m_object; }

    void GetOnDisableVisibility(IVariable* var);

    const CVarObject* GetVarObject() const { return &m_vars; }
    CVarObject* GetVarObject() { return &m_vars; }

    virtual void Update();


    void UseWeaponIK([[maybe_unused]] bool val) { m_weaponIK = true; }

    // Set current material to render object.
    void SetCustomMaterial(CMaterial* pMaterial);
    // Get custom material that object is rendered with.
    CMaterial* GetCustomMaterial() { return m_pCurrentMaterial; };

    // Get material the object is actually rendered with.
    CMaterial* GetMaterial();

    void ReleaseObject();
    void RePhysicalize();

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Vec3 m_GridOrigin;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    void SetPaused(bool bPaused);
    bool GetPaused() {return m_bPaused; }

    bool IsCameraAttached() const{ return mv_AttachCamera; }

    virtual void PlayAnimation(const char* szName);

    const QString& GetLoadedFileName() const { return m_loadedFile; }

    void Physicalize();

protected:

    void LoadStaticObject(const QString& file);

    // Called to render stuff.
    virtual void OnRender();

    virtual void DrawFloorGrid(const Quat& tmRotation, const Vec3& MotionTranslation, const Matrix33& rGridRot);
    void DrawCoordSystem(const QuatT& q, f32 length);

    void SaveDebugOptions() const;
    void RestoreDebugOptions();

    virtual void DrawModel(const SRenderingPassInfo& passInfo);
    virtual void DrawLights(const SRenderingPassInfo& passInfo);
    virtual void DrawSkyBox(const SRenderingPassInfo& passInfo);

    void DrawInfo() const;

    void SetConsoleVar(const char* var, int value);

    void OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        if (event != eNotify_OnBeginGameMode)
        {
            // the base class responds to this by forcing itself to be the current context.
            // we don't want that to be the case for previewer viewports.
            CRenderViewport::OnEditorNotifyEvent(event);
        }
    }

    IStatObj* m_object;
    IStatObj* m_weaponModel;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING

    QString m_attachBone;
    AABB m_AABB;

    struct BBox
    {
        OBB obb;
        Vec3 pos;
        ColorB col;
    };
    std::vector<BBox>  m_arrBBoxes;

    // Camera control.
    float m_camRadius;

    // True to show grid.
    bool m_bGrid;
    bool m_bBase;

    QString m_settingsPath;

    bool m_weaponIK;

    QString m_loadedFile;
    CDLight m_VPLight;

    f32 m_LightRotationRadian;

    class CRESky* m_pRESky;
    struct ICVar* m_pSkyboxName;
    IShader* m_pSkyBoxShader;
    _smart_ptr<CMaterial> m_pCurrentMaterial;

    //---------------------------------------------------
    //---    debug options                            ---
    //---------------------------------------------------
    CVariable<bool> mv_showGrid;
    CVariable<bool> mv_showBase;
    CVariable<bool> mv_showLocator;
    CVariable<bool> mv_InPlaceMovement;
    CVariable<bool> mv_StrafingControl;

    CVariable<bool> mv_showWireframe1;  //draw wireframe instead of solid-geometry.
    CVariable<bool> mv_showWireframe2;  //this one is software-wireframe rendered on top of the solid geometry
    CVariable<bool> mv_showTangents;
    CVariable<bool> mv_showBinormals;
    CVariable<bool> mv_showNormals;

    CVariable<bool> mv_showSkeleton;
    CVariable<bool> mv_showJointNames;
    CVariable<bool> mv_showJointsValues;
    CVariable<bool> mv_showStartLocation;
    CVariable<bool> mv_showMotionParam;
    CVariable<float> mv_UniformScaling;

    CVariable<bool> mv_printDebugText;
    CVariable<bool> mv_AttachCamera;

    CVariable<bool> mv_showShaders;

    CVariable<bool> mv_lighting;
    CVariable<bool> mv_animateLights;

    CVariable<Vec3> mv_backgroundColor;
    CVariable<Vec3> mv_objectAmbientColor;

    CVariable<Vec3> mv_lightDiffuseColor;
    CVariable<float> mv_lightMultiplier;
    CVariable<float> mv_lightSpecMultiplier;
    CVariable<float> mv_lightRadius;
    CVariable<float> mv_lightOrbit;

    CVariable<float> mv_fov;
    CVariable<bool> mv_showPhysics;
    CVariable<bool> mv_useCharPhysics;
    CVariable<bool> mv_showPhysicsTetriders;
    CVariable<int> mv_forceLODNum;

    CVariableArray mv_advancedTable;

    CVarObject m_vars;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

public slots:
    virtual void OnAnimPlay();
    virtual void OnAnimBack();
    virtual void OnAnimFastBack();
    virtual void OnAnimFastForward();
    virtual void OnAnimFront();
protected:
    bool m_bPaused;

    void OnDestroy();
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    struct VariableCallbackIndex
    {
        enum : unsigned char
        {
            OnCharPhysics = 0,
            OnLightColor,
            OnLightMultiplier,
            OnShowShaders,

            // must be at the end
            Count,
        };
    };
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AZStd::fixed_vector< IVariable::OnSetCallback, VariableCallbackIndex::Count > m_onSetCallbacksCache;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_MODELVIEWPORT_H
