/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

            EMotionFX::Actor*                           mActor;
            MCore::Array<uint32>                        mBoneList;
            RenderGL::GLActor*                          mRenderActor;
            MCore::Array<EMotionFX::ActorInstance*>     mActorInstances;
            float                                       mNormalsScaleMultiplier;
            float                                       mCharacterHeight;
            float                                       mOffsetFromTrajectoryNode;
            bool                                        mMustCalcNormalScale;

            EMStudioRenderActor(EMotionFX::Actor* actor, RenderGL::GLActor* renderActor);
            virtual ~EMStudioRenderActor();

            void CalculateNormalScaleMultiplier();
        };

        class Layout
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderPlugin::Layout, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

        public:
            Layout() { mRenderPlugin = nullptr; }
            virtual ~Layout() { }
            virtual QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) = 0;
            virtual const char* GetName() = 0;
            virtual const char* GetImageFileName() = 0;

        private:
            RenderPlugin* mRenderPlugin;
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
        void ZoomToJoints(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::Node*>& joints);

        // ActorNotificationBus
        void OnActorReady(EMotionFX::Actor* actor) override;

        EMStudioPlugin::EPluginType GetPluginType() const override              { return EMStudioPlugin::PLUGINTYPE_RENDERING; }
        uint32 GetProcessFramePriority() const override                         { return 100; }

        PluginOptions* GetOptions() override { return &mRenderOptions; }

        // render actors
        EMStudioRenderActor* FindEMStudioActor(EMotionFX::ActorInstance* actorInstance, bool doubleCheckInstance = true);
        EMStudioRenderActor* FindEMStudioActor(EMotionFX::Actor* actor);
        uint32 FindEMStudioActorIndex(EMStudioRenderActor* EMStudioRenderActor);

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
        MCORE_INLINE MCommon::TranslateManipulator* GetTranslateManipulator()       { return mTranslateManipulator; }
        MCORE_INLINE MCommon::RotateManipulator* GetRotateManipulator()             { return mRotateManipulator; }
        MCORE_INLINE MCommon::ScaleManipulator* GetScaleManipulator()               { return mScaleManipulator; }

        // other helpers
        MCORE_INLINE RenderOptions* GetRenderOptions()                              { return &mRenderOptions; }

        // view widget helpers
        MCORE_INLINE RenderViewWidget* GetFocusViewWidget()                         { return mFocusViewWidget; }
        MCORE_INLINE void SetFocusViewWidget(RenderViewWidget* focusViewWidget)     { mFocusViewWidget = focusViewWidget; }

        RenderViewWidget* GetViewWidget(size_t index) { return m_viewWidgets[index]; }
        size_t GetNumViewWidgets() const { return m_viewWidgets.size(); }
        RenderViewWidget* CreateViewWidget(QWidget* parent);
        void RemoveViewWidget(RenderViewWidget* viewWidget);
        void ClearViewWidgets();

        MCORE_INLINE RenderViewWidget* GetActiveViewWidget()                        { return mActiveViewWidget; }
        MCORE_INLINE void SetActiveViewWidget(RenderViewWidget* viewWidget)         { mActiveViewWidget = viewWidget; }

        void AddLayout(Layout* layout) { m_layouts.emplace_back(layout); }
        Layout* FindLayoutByName(const AZStd::string& layoutName) const;
        Layout* GetCurrentLayout() const { return m_currentLayout; }
        const AZStd::vector<Layout*>& GetLayouts() { return m_layouts; }

        MCORE_INLINE QCursor& GetZoomInCursor()                                     { assert(mZoomInCursor); return *mZoomInCursor; }
        MCORE_INLINE QCursor& GetZoomOutCursor()                                    { assert(mZoomOutCursor); return *mZoomOutCursor; }

        MCORE_INLINE CommandSystem::SelectionList* GetCurrentSelection() const      { return mCurrentSelection; }
        MCORE_INLINE MCommon::RenderUtil* GetRenderUtil() const                     { return mRenderUtil; }

        MCore::AABB GetSceneAABB(bool selectedInstancesOnly);

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
        MCommon::TranslateManipulator*      mTranslateManipulator;
        MCommon::RotateManipulator*         mRotateManipulator;
        MCommon::ScaleManipulator*          mScaleManipulator;

        MCommon::RenderUtil*                mRenderUtil;
        RenderUpdateCallback*               mUpdateCallback;

        RenderOptions                       mRenderOptions;
        MCore::Array<EMStudioRenderActor*>  mActors;

        // view widgets
        AZStd::vector<RenderViewWidget*>    m_viewWidgets;
        RenderViewWidget*                   mActiveViewWidget;
        RenderViewWidget*                   mFocusViewWidget;

        // render view layouts
        AZStd::vector<Layout*>              m_layouts;
        Layout*                             m_currentLayout;

        // cursor image files
        QCursor*                            mZoomInCursor;
        QCursor*                            mZoomOutCursor;

        // window visibility
        bool                                mIsVisible;

        // base layout and interface functionality
        QHBoxLayout*                        mBaseLayout;
        QWidget*                            mRenderLayoutWidget;
        QWidget*                            mInnerWidget;
        CommandSystem::SelectionList*       mCurrentSelection;
        bool                                mFirstFrameAfterReInit;
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
        UpdateRenderActorsCallback*         mUpdateRenderActorsCallback;
        ReInitRenderActorsCallback*         mReInitRenderActorsCallback;
        CreateActorInstanceCallback*        mCreateActorInstanceCallback;
        RemoveActorInstanceCallback*        mRemoveActorInstanceCallback;
        SelectCallback*                     mSelectCallback;
        UnselectCallback*                   mUnselectCallback;
        ClearSelectionCallback*             mClearSelectionCallback;
        CommandResetToBindPoseCallback*     mResetToBindPoseCallback;
        AdjustActorInstanceCallback*        mAdjustActorInstanceCallback;
    };
} // namespace EMStudio
