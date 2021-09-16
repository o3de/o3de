/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include "../EMStudioConfig.h"
#include "../DockWidgetPlugin.h"
#include "RenderOptions.h"
#include "RenderWidget.h"
#include "RenderUpdateCallback.h"
#include "RenderViewWidget.h"
#include <EMotionFX/Rendering/Common/Camera.h>
#include <EMotionFX/Rendering/Common/TranslateManipulator.h>
#include <EMotionFX/Rendering/Common/RotateManipulator.h>
#include <EMotionFX/Rendering/Common/ScaleManipulator.h>
#include <EMotionFX/Rendering/OpenGL2/Source/glactor.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/ImporterCommands.h>
#include <Source/Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QWidget>
#include <QSignalMapper>
#endif


namespace EMStudio
{
#define DEFAULT_FLIGHT_TIME 1.0f

    class EMSTUDIO_API RenderPlugin
        : public DockWidgetPlugin
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
        , private EMotionFX::ActorNotificationBus::Handler
    {
        MCORE_MEMORYOBJECTCATEGORY(RenderPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE)
        Q_OBJECT // AUTOMOC

    public:
        enum
        {
            CLASS_ID = 0xa83f74a2
        };

        struct EMSTUDIO_API EMStudioRenderActor
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderPlugin::EMStudioRenderActor, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

            EMotionFX::Actor*                           m_actor;
            AZStd::vector<size_t>                        m_boneList;
            RenderGL::GLActor*                          m_renderActor;
            AZStd::vector<EMotionFX::ActorInstance*>     m_actorInstances;
            float                                       m_normalsScaleMultiplier;
            float                                       m_characterHeight;
            float                                       m_offsetFromTrajectoryNode;
            bool                                        m_mustCalcNormalScale;

            EMStudioRenderActor(EMotionFX::Actor* actor, RenderGL::GLActor* renderActor);
            virtual ~EMStudioRenderActor();

            void CalculateNormalScaleMultiplier();
        };

        class Layout
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderPlugin::Layout, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

        public:
            Layout() { m_renderPlugin = nullptr; }
            virtual ~Layout() { }
            virtual QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) = 0;
            virtual const char* GetName() = 0;
            virtual const char* GetImageFileName() = 0;

        private:
            RenderPlugin* m_renderPlugin;
        };

        RenderPlugin();
        virtual ~RenderPlugin();

        void Reflect(AZ::ReflectContext* context) override;
        bool Init() override;
        void OnAfterLoadProject() override;
        void OnAfterLoadActors() override;

        // callback functions
        virtual void CreateRenderWidget(RenderViewWidget* renderViewWidget, RenderWidget** outRenderWidget, QWidget** outWidget) = 0;
        virtual bool CreateEMStudioActor(EMotionFX::Actor* actor) = 0;

        // SkeletonOutlinerNotificationBus
        void ZoomToJoints(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::Node*>& joints) override;

        // ActorNotificationBus
        void OnActorReady(EMotionFX::Actor* actor) override;

        EMStudioPlugin::EPluginType GetPluginType() const override              { return EMStudioPlugin::PLUGINTYPE_RENDERING; }
        uint32 GetProcessFramePriority() const override                         { return 100; }

        PluginOptions* GetOptions() override { return &m_renderOptions; }

        // render actors
        EMStudioRenderActor* FindEMStudioActor(const EMotionFX::ActorInstance* actorInstance, bool doubleCheckInstance = true) const;
        EMStudioRenderActor* FindEMStudioActor(const EMotionFX::Actor* actor) const;
        size_t FindEMStudioActorIndex(const EMStudioRenderActor* EMStudioRenderActor) const;

        void AddEMStudioActor(EMStudioRenderActor* emstudioActor);
        bool DestroyEMStudioActor(EMotionFX::Actor* actor);

        // reinitializes the render actors
        void ReInit(bool resetViewCloseup = true);

        // view close up helpers
        void ViewCloseup(bool selectedInstancesOnly = true, RenderWidget* renderWidget = nullptr, float flightTime = DEFAULT_FLIGHT_TIME);
        void SetSkipFollowCalcs(bool skipFollowCalcs);

        // manipulators
        void ReInitTransformationManipulators();
        MCommon::TransformationManipulator* GetActiveManipulator(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY);
        MCORE_INLINE MCommon::TranslateManipulator* GetTranslateManipulator()       { return m_translateManipulator; }
        MCORE_INLINE MCommon::RotateManipulator* GetRotateManipulator()             { return m_rotateManipulator; }
        MCORE_INLINE MCommon::ScaleManipulator* GetScaleManipulator()               { return m_scaleManipulator; }

        // other helpers
        MCORE_INLINE RenderOptions* GetRenderOptions()                              { return &m_renderOptions; }

        // view widget helpers
        MCORE_INLINE RenderViewWidget* GetFocusViewWidget()                         { return m_focusViewWidget; }
        MCORE_INLINE void SetFocusViewWidget(RenderViewWidget* focusViewWidget)     { m_focusViewWidget = focusViewWidget; }

        RenderViewWidget* GetViewWidget(size_t index) { return m_viewWidgets[index]; }
        size_t GetNumViewWidgets() const { return m_viewWidgets.size(); }
        RenderViewWidget* CreateViewWidget(QWidget* parent);
        void RemoveViewWidget(RenderViewWidget* viewWidget);
        void ClearViewWidgets();

        MCORE_INLINE RenderViewWidget* GetActiveViewWidget()                        { return m_activeViewWidget; }
        MCORE_INLINE void SetActiveViewWidget(RenderViewWidget* viewWidget)         { m_activeViewWidget = viewWidget; }

        void AddLayout(Layout* layout) { m_layouts.emplace_back(layout); }
        Layout* FindLayoutByName(const AZStd::string& layoutName) const;
        Layout* GetCurrentLayout() const { return m_currentLayout; }
        const AZStd::vector<Layout*>& GetLayouts() { return m_layouts; }

        MCORE_INLINE QCursor& GetZoomInCursor()                                     { assert(m_zoomInCursor); return *m_zoomInCursor; }
        MCORE_INLINE QCursor& GetZoomOutCursor()                                    { assert(m_zoomOutCursor); return *m_zoomOutCursor; }

        MCORE_INLINE CommandSystem::SelectionList* GetCurrentSelection() const      { return m_currentSelection; }
        MCORE_INLINE MCommon::RenderUtil* GetRenderUtil() const                     { return m_renderUtil; }

        AZ::Aabb GetSceneAabb(bool selectedInstancesOnly);

        MCommon::RenderUtil::TrajectoryTracePath* FindTracePath(EMotionFX::ActorInstance* actorInstance);
        void ResetSelectedTrajectoryPaths();

        void ProcessFrame(float timePassedInSeconds) override;

        void UpdateActorInstances(float timePassedInSeconds);

        virtual void UpdateActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds);
        virtual void RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds);
        virtual void ResetCameras();

        void SaveRenderOptions();
        void LoadRenderOptions();

        inline static constexpr AZStd::string_view s_renderWindowShortcutGroupName = "Render Window";
        inline static constexpr AZStd::string_view s_showSelectedShortcutName = "Show Selected";
        inline static constexpr AZStd::string_view s_showEntireSceneShortcutName = "Show Entire Scene";
        inline static constexpr AZStd::string_view s_toggleSelectionBoxRenderingShortcutName = "Toggle Selection Box Rendering";

    public slots:
        void SetManipulatorMode(RenderOptions::ManipulatorMode mode);
        void SetSelectionMode() { SetManipulatorMode(RenderOptions::ManipulatorMode::SELECT); }
        void SetTranslationMode() { SetManipulatorMode(RenderOptions::ManipulatorMode::TRANSLATE); }
        void SetRotationMode() { SetManipulatorMode(RenderOptions::ManipulatorMode::ROTATE); }
        void SetScaleMode() { SetManipulatorMode(RenderOptions::ManipulatorMode::SCALE); }

        void CleanEMStudioActors();

        void VisibilityChanged(bool visible);

        void LayoutButtonPressed(const QString& text);

    protected:
        //void SetAspiredRenderingFPS(int32 fps);

        // motion extraction paths
        AZStd::vector<MCommon::RenderUtil::TrajectoryTracePath*> m_trajectoryTracePaths;

        // the transformation manipulators
        MCommon::TranslateManipulator*      m_translateManipulator;
        MCommon::RotateManipulator*         m_rotateManipulator;
        MCommon::ScaleManipulator*          m_scaleManipulator;

        MCommon::RenderUtil*                m_renderUtil;
        RenderUpdateCallback*               m_updateCallback;

        RenderOptions                       m_renderOptions;
        AZStd::vector<EMStudioRenderActor*>  m_actors;

        // view widgets
        AZStd::vector<RenderViewWidget*>    m_viewWidgets;
        RenderViewWidget*                   m_activeViewWidget;
        RenderViewWidget*                   m_focusViewWidget;

        // render view layouts
        AZStd::vector<Layout*>              m_layouts;
        Layout*                             m_currentLayout;

        // cursor image files
        QCursor*                            m_zoomInCursor;
        QCursor*                            m_zoomOutCursor;

        // window visibility
        bool                                m_isVisible;

        // base layout and interface functionality
        QHBoxLayout*                        m_baseLayout;
        QWidget*                            m_renderLayoutWidget;
        QWidget*                            m_innerWidget;
        CommandSystem::SelectionList*       m_currentSelection;
        bool                                m_firstFrameAfterReInit;
        bool                                m_reinitRequested = false;

        // command callbacks
        MCORE_DEFINECOMMANDCALLBACK(UpdateRenderActorsCallback);
        MCORE_DEFINECOMMANDCALLBACK(ReInitRenderActorsCallback);
        MCORE_DEFINECOMMANDCALLBACK(CreateActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(RemoveActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(SelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(UnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(ClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandResetToBindPoseCallback);
        MCORE_DEFINECOMMANDCALLBACK(AdjustActorInstanceCallback);
        UpdateRenderActorsCallback*         m_updateRenderActorsCallback;
        ReInitRenderActorsCallback*         m_reInitRenderActorsCallback;
        CreateActorInstanceCallback*        m_createActorInstanceCallback;
        RemoveActorInstanceCallback*        m_removeActorInstanceCallback;
        SelectCallback*                     m_selectCallback;
        UnselectCallback*                   m_unselectCallback;
        ClearSelectionCallback*             m_clearSelectionCallback;
        CommandResetToBindPoseCallback*     m_resetToBindPoseCallback;
        AdjustActorInstanceCallback*        m_adjustActorInstanceCallback;
    };
} // namespace EMStudio
