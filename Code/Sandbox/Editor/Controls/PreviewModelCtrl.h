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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_PREVIEWMODELCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_PREVIEWMODELCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QPoint>
#include <QWidget>

#include <IStatObj.h>

#include <Editor/Material/Material.h>

#endif

struct IRenderNode;
class CImageEx;

class CPreviewModelCtrl
    : public QWidget
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    explicit CPreviewModelCtrl(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    QSize minimumSizeHint() const override;

public:
    void LoadFile(const QString& modelFile, bool changeCamera = true);
    Vec3 GetSize() const { return m_size; };
    QString GetLoadedFile() const { return m_loadedFile; }

    void SetEntity(IRenderNode* entity);
    void SetObject(IStatObj* pObject);
    IStatObj* GetObject() { return m_pObj; }
    void SetCameraLookAt(float fRadiusScale, const Vec3& dir = Vec3(0, 1, 0));
    void SetCameraRadius(float fRadius);
    CCamera& GetCamera();
    void SetGrid(bool bEnable) { m_bGrid = bEnable; }
    void SetAxis(bool bEnable, bool forParticleEditor = false) { m_bAxis = bEnable; m_bAxisParticleEditor = forParticleEditor; }
    void SetRotation(bool bEnable);
    void SetClearColor(const ColorF& color);
    void SetBackgroundTexture(const QString& textureFilename);
    void UseBackLight(bool bEnable);
    bool UseBackLight() const { return m_bUseBacklight; }
    void SetShowNormals(bool bShow) { m_bShowNormals = bShow; }
    void SetShowPhysics(bool bShow) { m_bShowPhysics = bShow; }
    void SetShowRenderInfo(bool bShow) { m_bShowRenderInfo = bShow; }

    void EnableUpdate(bool bEnable);
    bool IsUpdateEnabled() const { return m_bUpdate; }
    void Update(bool bForceUpdate = false);
    void ProcessKeys();

    // this turns on and off aspect-ratio-maintaining.  Use it when the widget is free to resize itself.
    void SetAspectRatio(float newAspectRatio);
    int heightForWidth(int w) const override;
    bool hasHeightForWidth() const override;

    void SetMaterial(CMaterial* pMaterial);
    CMaterial*  GetMaterial();

    void GetImageOffscreen(CImageEx& image, const QSize& customSize = QSize(0, 0));

    void GetCameraTM(Matrix34& cameraTM);
    void SetCameraTM(const Matrix34& cameraTM);

    // Place camera so that whole object fits on screen.
    void FitToScreen();

    // Get information about the preview model.
    int GetFaceCount();
    int GetVertexCount();
    int GetMaxLod();
    int GetMtlCount();

    void SetShowObject(bool bShowObject) {m_bShowObject = bShowObject; }
    bool GetShowObject() {return m_bShowObject; }

    void SetAmbient(ColorF amb) { m_ambientColor = amb; }
    void SetAmbientMultiplier(f32 multiplier) { m_ambientMultiplier = multiplier; }

    typedef void (* CameraChangeCallback)(void* m_userData, CPreviewModelCtrl* m_currentCamera);
    void SetCameraChangeCallback(CameraChangeCallback callback, void* userData) { m_cameraChangeCallback = callback, m_pCameraChangeUserData = userData; }

    void EnableMaterialPrecaching(bool bPrecacheMaterial) { m_bPrecacheMaterial = bPrecacheMaterial; }
    void EnableWireframeRendering(bool bDrawWireframe) { m_bDrawWireFrame = bDrawWireframe; }

public:
    ~CPreviewModelCtrl();

    bool CreateContext();
    void ReleaseObject();
    void DeleteRenderContex();

protected:
    void OnCreate();
    void OnDestroy();
    void OnLButtonDown(QPoint point);
    void OnLButtonUp(QPoint point);
    void OnMButtonDown(QPoint point);
    void OnMButtonUp(QPoint point);
    void OnRButtonUp(QPoint point);
    void OnRButtonDown(QPoint point);

    QPaintEngine* paintEngine() const override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:
    virtual bool Render();
    virtual void SetCamera(CCamera& cam);

    virtual void RenderObject(_smart_ptr<IMaterial> pMaterial, SRenderingPassInfo& passInfo);

    HWND m_hWnd;
    CCamera m_camera;
    float m_fov;

    struct SPreviousContext;
    std::vector<SPreviousContext> m_previousContexts;

    void SetOrbitAngles(const Ang3& ang);
    void DrawGrid();
    void DrawBackground();
    _smart_ptr<IMaterial> GetCurrentMaterial();

    _smart_ptr<IStatObj> m_pObj;

    IRenderer* m_pRenderer;
    bool m_bContextCreated;

    Vec3 m_size;
    Vec3 m_pos;
    int m_nTimer;
    bool m_useAspectRatio = false;
    float m_aspectRatio = 1.0f;

    QString m_loadedFile;
    std::vector<CDLight> m_lights;

    AABB m_aabb;
    Vec3 m_cameraTarget;
    float m_cameraRadius;
    Vec3 m_cameraAngles;
    bool m_bInRotateMode;
    bool m_bInMoveMode;
    bool m_bInPanMode;
    QPoint m_mousePosition;
    QPoint m_previousMousePosition;
    IRenderNode* m_pEntity;
    bool m_bHaveAnythingToRender;
    bool m_bGrid;
    bool m_bAxis;
    bool m_bAxisParticleEditor;
    bool m_bUpdate;
    bool m_bRotate;
    float m_rotateAngle;
    ColorF m_clearColor;
    ColorF m_ambientColor;
    f32 m_ambientMultiplier;
    bool m_bUseBacklight;
    bool m_bShowObject;
    bool m_bPrecacheMaterial;
    bool m_bDrawWireFrame;
    bool m_bShowNormals;
    bool m_bShowPhysics;
    bool m_bShowRenderInfo;
    int m_backgroundTextureId;
    float m_tileX;
    float m_tileY;
    float m_tileSizeX;
    float m_tileSizeY;
    _smart_ptr<CMaterial> m_pCurrentMaterial;
    CameraChangeCallback m_cameraChangeCallback;
    void*   m_pCameraChangeUserData;

protected:
    void StorePreviousContext();
    void SetCurrentContext();
    void RestorePreviousContext();
};
#endif // CRYINCLUDE_EDITOR_CONTROLS_PREVIEWMODELCTRL_H
