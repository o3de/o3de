/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Math/Aabb.h>
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
            AZ::Vector3  m_posA;
            AZ::Vector3  m_posB;
            AZ::Vector3  m_posC;

            AZ::Vector3  m_normalA;
            AZ::Vector3  m_normalB;
            AZ::Vector3  m_normalC;

            uint32          m_color;

            Triangle() {}
            Triangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)
                : m_posA(posA)
                , m_posB(posB)
                , m_posC(posC)
                , m_normalA(normalA)
                , m_normalB(normalB)
                , m_normalC(normalC)
                , m_color(color) {}
        };


        class EventHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            EventHandler(RenderWidget* widget)
                : EMotionFX::EventHandler() { m_widget = widget; }
            ~EventHandler() {}

            // overloaded
            const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_DRAW_LINE, EMotionFX::EVENT_TYPE_ON_DRAW_TRIANGLE, EMotionFX::EVENT_TYPE_ON_DRAW_TRIANGLES }; }
            MCORE_INLINE void OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color) override         { m_widget->AddTriangle(posA, posB, posC, normalA, normalB, normalC, color); }
            MCORE_INLINE void OnDrawTriangles() override                                                                                                                                                                                                     { m_widget->RenderTriangles(); }

        private:
            RenderWidget*   m_widget;
        };

        RenderWidget(RenderPlugin* renderPlugin, RenderViewWidget* viewWidget);
        virtual ~RenderWidget();

        void CreateActions();

        // main render callback
        virtual void Render() = 0;
        virtual void Update() = 0;

        // line rendering helper functions
        MCORE_INLINE void AddTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)        { m_triangles.emplace_back(Triangle(posA, posB, posC, normalA, normalB, normalC, color)); }
        MCORE_INLINE void ClearTriangles()                                                                  { m_triangles.clear(); }
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
        MCORE_INLINE MCommon::Camera* GetCamera() const                                                     { return m_camera; }
        MCORE_INLINE CameraMode GetCameraMode() const                                                       { return m_cameraMode; }
        MCORE_INLINE void SetSkipFollowCalcs(bool skipFollowCalcs)                                          { m_skipFollowCalcs = skipFollowCalcs; }
        void ViewCloseup(const AZ::Aabb& aabb, float flightTime, uint32 viewCloseupWaiting = 5);
        void ViewCloseup(bool selectedInstancesOnly, float flightTime, uint32 viewCloseupWaiting = 5);
        void SwitchCamera(CameraMode mode);

        // render bugger dimensions
        MCORE_INLINE uint32 GetScreenWidth() const                                                          { return m_width; }
        MCORE_INLINE uint32 GetScreenHeight() const                                                         { return m_height; }

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

        RenderPlugin*                           m_plugin;
        RenderViewWidget*                       m_viewWidget;
        AZStd::vector<Triangle>                  m_triangles;
        EventHandler                            m_eventHandler;

        AZStd::vector<EMotionFX::ActorInstance*> m_selectedActorInstances;

        MCommon::TransformationManipulator*     m_activeTransformManip;

        // camera helper data
        CameraMode                              m_cameraMode;
        MCommon::Camera*                        m_camera;
        MCommon::Camera*                        m_axisFakeCamera;
        bool                                    m_isCharacterFollowModeActive;
        bool                                    m_skipFollowCalcs;
        bool                                    m_needDisableFollowMode;

        // render buffer dimensions
        uint32                                  m_width;
        uint32                                  m_height;

        // used for closeup camera flights
        uint32                                  m_viewCloseupWaiting;
        AZ::Aabb                                m_viewCloseupAabb;
        float                                   m_viewCloseupFlightTime;

        // manipulator helper data
        AZ::Vector3                          m_oldActorInstancePos;
        int32                                   m_prevMouseX;
        int32                                   m_prevMouseY;
        int32                                   m_prevLocalMouseX;
        int32                                   m_prevLocalMouseY;
        int32                                   m_rightClickPosX;
        int32                                   m_rightClickPosY;
        int32                                   m_pixelsMovedSinceRightClick;
    };
} // namespace EMStudio
