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

#pragma once

//Editor
#if !defined(Q_MOC_RUN)
#include "IEditor.h"
#include "../EditorCommon/QViewport.h"
#include "../EditorCommon/QViewportConsumer.h"

#endif

struct IStatObj;
struct SRenderingPassInfo;
struct SRendParams;
class CParticleItem;
class CAxisHelper;
struct HitContext;
struct IGizmoMouseDragHandler;
struct SLodInfo;

namespace RotationDrawHelper
{
    class Axis;
}

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class CPreviewModelView
    : public QViewport
    , public QViewportConsumer
    , public IEditorNotifyListener
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    enum class PlayState
    {
        NONE,
        PLAY,
        PAUSE,
        STEP,
        RESET
    };
    enum class SplineMode
    {
        NONE,
        LINE,
        SINEWAVE,
        COIL
    };

    //NOTE: these need to be limited to 32 as these are being used for bit wise operations.
    enum class PreviewModelViewFlag
    {
        DRAW_WIREFRAME = 0,

        SHOW_BOUNDINGBOX,
        SHOW_GIZMO,
        SHOW_GRID,
        SHOW_GRID_AXIS,
        SHOW_EMITTER_SHAPE,
        SHOW_OVERDRAW,
        SHOW_FIRST_CONTAINER,

        LOOPING_PLAY,

        //Spline
        SPLINE_LOOPING,
        SPLINE_PINGPONG,

        PRECACHE_MATERIAL,

        ENABLE_TIME_OF_DAY,

        END_POSSIBLE_ITEMS = 32,
    };

    explicit CPreviewModelView(QWidget* parent);
    virtual ~CPreviewModelView();

    //Flags
    bool IsFlagSet(PreviewModelViewFlag flag) const;
    void ToggleFlag(PreviewModelViewFlag flag);
    virtual void SetFlag(PreviewModelViewFlag flag);
    virtual void UnSetFlag(PreviewModelViewFlag flag);

    //Resets
    void ResetPlaybackControls();
    void ResetBackgroundColor();
    void ResetGridColor();
    virtual void ResetCamera();
    virtual void ResetAll();

    void SetGridColor(ColorF color);
    void SetBackgroundColor(ColorF color);
    void SetPlayState(PlayState state);
    void SetTimeScale(float scale);

    PlayState GetPlayState() const;
    float GetTimeScale() const;
    ColorF GetGridColor() const;
    ColorF GetBackgroundColor() const;

    void ImportModel();

    void LoadModelFile(const QString& modelFile);
    IStatObj* GetStaticModel();

    typedef std::function<void()> PostUpdateCallback;
    void SetPostUpdateCallback(PostUpdateCallback callback);

    typedef std::function<void(Vec2i mousePos)> ContextMenuCallback;
    void SetContextMenuCallback(ContextMenuCallback callback);

    ////////////////////////////////////////////////////////
    //QViewportConsumer
    virtual void OnViewportRender(const SRenderContext& rc) override;
    virtual void OnViewportKey(const SKeyEvent& ev) override;
    virtual void OnViewportMouse(const SMouseEvent& ev) override;
    ////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////
    //IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    ////////////////////////////////////////////////////////

private:

    //Update
    void UpdateSettings();
protected:
private:
    //Render
    void RenderModels(SRendParams& rendParams, SRenderingPassInfo& passInfo);

    //Misc
    void ReleaseModel();
    void SetDefaultFlags();
protected:
    void FocusOnScreen();
private:
    float GetSpeedScale() const;

private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    ColorF m_GridColor;
    ColorF m_BackgroundColor;

    QString m_ModelFilename;


    IStatObj* m_pStaticModel;

protected:
    PostUpdateCallback m_PostUpdateCallback;
    ContextMenuCallback m_ContextMenuCallback;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

protected:
    PlayState m_PlayState;
    float m_TimeScale;
private:
    unsigned int m_Flags;
};
