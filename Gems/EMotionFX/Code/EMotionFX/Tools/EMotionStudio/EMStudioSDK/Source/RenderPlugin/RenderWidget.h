/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_RENDERWIDGET_H
#define __EMSTUDIO_RENDERWIDGET_H

//
#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include "../EMStudioConfig.h"
#include <EMotionFX/Rendering/Common/Camera.h>
#include <EMotionFX/Rendering/Common/TransformationManipulator.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/EventManager.h>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#endif


namespace EMStudio
{
    // forward declaration
    class RenderPlugin;
    class RenderViewWidget;

    class EMSTUDIO_API RenderWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(RenderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        enum CameraMode
        {
            CAMMODE_ORBIT       = 0,
            CAMMODE_FIRSTPERSON = 1,
            CAMMODE_FRONT       = 2,
            CAMMODE_BACK        = 3,
            CAMMODE_LEFT        = 4,
            CAMMODE_RIGHT       = 5,
            CAMMODE_TOP         = 6,
            CAMMODE_BOTTOM      = 7
        };

        struct Triangle
        {
            AZ::Vector3  mPosA;
            AZ::Vector3  mPosB;
            AZ::Vector3  mPosC;

            AZ::Vector3  mNormalA;
            AZ::Vector3  mNormalB;
            AZ::Vector3  mNormalC;

            uint32          mColor;

            Triangle() {}
            Triangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)
                : mPosA(posA)
                , mPosB(posB)
                , mPosC(posC)
                , mNormalA(normalA)
                , mNormalB(normalB)
                , mNormalC(normalC)
                , mColor(color) {}
        };


        class EventHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            EventHandler(RenderWidget* widget)
                : EMotionFX::EventHandler() { mWidget = widget; }
            ~EventHandler() {}

            // overloaded
            const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_DRAW_LINE, EMotionFX::EVENT_TYPE_ON_DRAW_TRIANGLE, EMotionFX::EVENT_TYPE_ON_DRAW_TRIANGLES }; }
            MCORE_INLINE void OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)         { mWidget->AddTriangle(posA, posB, posC, normalA, normalB, normalC, color); }
            MCORE_INLINE void OnDrawTriangles()                                                                                                                                                                                                     { mWidget->RenderTriangles(); }

        private:
            RenderWidget*   mWidget;
        };

        RenderWidget(RenderPlugin* renderPlugin, RenderViewWidget* viewWidget);
        virtual ~RenderWidget();

        void CreateActions();

        // main render callback
        virtual void Render() = 0;
        virtual void Update() = 0;

        // line rendering helper functions
        MCORE_INLINE void AddTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)        { mTriangles.Add(Triangle(posA, posB, posC, normalA, normalB, normalC, color)); }
        MCORE_INLINE void ClearTriangles()                                                                  { mTriangles.Clear(false); }
        void RenderTriangles();

        // helper rendering functions
        void RenderActorInstances();
        void RenderGrid();
        void RenderCustomPluginData();
        void RenderAxis();
        void RenderNodeFilterString();
        void RenderManipulators();
        void RenderDebugDraw();
        void UpdateCamera();

        // camera helper functions
        MCORE_INLINE MCommon::Camera* GetCamera() const                                                     { return mCamera; }
        MCORE_INLINE CameraMode GetCameraMode() const                                                       { return mCameraMode; }
        MCORE_INLINE void SetSkipFollowCalcs(bool skipFollowCalcs)                                          { mSkipFollowCalcs = skipFollowCalcs; }
        void ViewCloseup(const MCore::AABB& aabb, float flightTime, uint32 viewCloseupWaiting = 5);
        void ViewCloseup(bool selectedInstancesOnly, float flightTime, uint32 viewCloseupWaiting = 5);
        void SwitchCamera(CameraMode mode);

        // render bugger dimensions
        MCORE_INLINE uint32 GetScreenWidth() const                                                          { return mWidth; }
        MCORE_INLINE uint32 GetScreenHeight() const                                                         { return mHeight; }

        // helper functions for easy calling
        void OnMouseMoveEvent(QWidget* renderWidget, QMouseEvent* event);
        void OnMousePressEvent(QWidget* renderWidget, QMouseEvent* event);
        void OnMouseReleaseEvent(QWidget* renderWidget, QMouseEvent* event);
        void OnWheelEvent(QWidget* renderWidget, QWheelEvent* event);
        void OnContextMenuEvent(QWidget* renderWidget, bool shiftPressed, bool altPressed, int32 localMouseX, int32 localMouseY, QPoint globalMousePos);

    protected:
        void UpdateActiveTransformationManipulator(MCommon::TransformationManipulator* activeManipulator);
        void UpdateCharacterFollowModeData();

        void closeEvent(QCloseEvent* event);

        RenderPlugin*                           mPlugin;
        RenderViewWidget*                       mViewWidget;
        MCore::Array<Triangle>                  mTriangles;
        EventHandler                            mEventHandler;

        MCore::Array<EMotionFX::ActorInstance*> mSelectedActorInstances;

        MCommon::TransformationManipulator*     mActiveTransformManip;

        // camera helper data
        CameraMode                              mCameraMode;
        MCommon::Camera*                        mCamera;
        MCommon::Camera*                        mAxisFakeCamera;
        bool                                    mIsCharacterFollowModeActive;
        bool                                    mSkipFollowCalcs;
        bool                                    mNeedDisableFollowMode;

        // render buffer dimensions
        uint32                                  mWidth;
        uint32                                  mHeight;

        // used for closeup camera flights
        uint32                                  mViewCloseupWaiting;
        MCore::AABB                             mViewCloseupAABB;
        float                                   mViewCloseupFlightTime;

        // manipulator helper data
        AZ::Vector3                          mOldActorInstancePos;
        int32                                   mPrevMouseX;
        int32                                   mPrevMouseY;
        int32                                   mPrevLocalMouseX;
        int32                                   mPrevLocalMouseY;
        int32                                   mRightClickPosX;
        int32                                   mRightClickPosY;
        int32                                   mPixelsMovedSinceRightClick;
    };
} // namespace EMStudio


#endif
