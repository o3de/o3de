/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/RenderPlugin/ManipulatorCallbacks.h>
#include <EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderLayouts.h>
#include <EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderPlugin.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MysticQt/Source/KeyboardShortcutManager.h>
#include <QDir>
#include <QMessageBox>


namespace EMStudio
{
    RenderPlugin::RenderPlugin()
        : DockWidgetPlugin()
    {
        mActors.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

        mIsVisible                      = true;
        mRenderUtil                     = nullptr;
        mUpdateCallback                 = nullptr;

        mUpdateRenderActorsCallback     = nullptr;
        mReInitRenderActorsCallback     = nullptr;
        mCreateActorInstanceCallback    = nullptr;
        mRemoveActorInstanceCallback    = nullptr;
        mSelectCallback                 = nullptr;
        mUnselectCallback               = nullptr;
        mClearSelectionCallback         = nullptr;
        mResetToBindPoseCallback        = nullptr;
        mAdjustActorInstanceCallback    = nullptr;

        mZoomInCursor                   = nullptr;
        mZoomOutCursor                  = nullptr;

        mBaseLayout                     = nullptr;
        mRenderLayoutWidget             = nullptr;
        mActiveViewWidget               = nullptr;
        mCurrentSelection               = nullptr;
        m_currentLayout                 = nullptr;
        mFocusViewWidget                = nullptr;
        mFirstFrameAfterReInit          = false;

        mTranslateManipulator           = nullptr;
        mRotateManipulator              = nullptr;
        mScaleManipulator               = nullptr;

        EMotionFX::ActorNotificationBus::Handler::BusConnect();
    }


    RenderPlugin::~RenderPlugin()
    {
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusDisconnect();
        EMotionFX::ActorNotificationBus::Handler::BusDisconnect();

        SaveRenderOptions();
        CleanEMStudioActors();

        // Get rid of the OpenGL view widgets.
        // Don't delete them directly as there might be still paint events in the Qt message queue which will cause a crash.
        // deleteLater will make sure all events will be processed before actually destructing the object.
        for (RenderViewWidget* viewWidget : m_viewWidgets)
        {
            viewWidget->deleteLater();
        }

        for (Layout* layout : m_layouts)
        {
            delete layout;
        }
        m_layouts.clear();

        // delete the gizmos
        GetManager()->RemoveTransformationManipulator(mTranslateManipulator);
        GetManager()->RemoveTransformationManipulator(mRotateManipulator);
        GetManager()->RemoveTransformationManipulator(mScaleManipulator);

        delete mTranslateManipulator;
        delete mRotateManipulator;
        delete mScaleManipulator;

        // get rid of the cursors
        delete mZoomInCursor;
        delete mZoomOutCursor;

        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mUpdateRenderActorsCallback, false);
        GetCommandManager()->RemoveCommandCallback(mReInitRenderActorsCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mResetToBindPoseCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustActorInstanceCallback, false);

        delete mUpdateRenderActorsCallback;
        delete mReInitRenderActorsCallback;
        delete mCreateActorInstanceCallback;
        delete mRemoveActorInstanceCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mResetToBindPoseCallback;
        delete mAdjustActorInstanceCallback;

        for (MCommon::RenderUtil::TrajectoryTracePath* trajectoryPath : m_trajectoryTracePaths)
        {
            delete trajectoryPath;
        }
        m_trajectoryTracePaths.clear();
    }


    // get rid of the render actors
    void RenderPlugin::CleanEMStudioActors()
    {
        // get rid of the actors
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            if (mActors[i])
            {
                delete mActors[i];
            }
        }
        mActors.Clear();
    }


    // get rid of the given render actor
    bool RenderPlugin::DestroyEMStudioActor(EMotionFX::Actor* actor)
    {
        // get the corresponding emstudio actor
        EMStudioRenderActor* emstudioActor = FindEMStudioActor(actor);
        if (emstudioActor == nullptr)
        {
            MCore::LogError("Cannot destroy render actor. There is no render actor registered for this actor.");
            return false;
        }

        // get the index of the emstudio actor, we can be sure it is valid as else the emstudioActor pointer would be nullptr already
        const uint32 index = FindEMStudioActorIndex(emstudioActor);
        MCORE_ASSERT(index != MCORE_INVALIDINDEX32);

        // get rid of the emstudio actor
        delete emstudioActor;
        mActors.Remove(index);
        return true;
    }


    // finds the manipulator the is currently hit by the mouse within the current camera
    MCommon::TransformationManipulator* RenderPlugin::GetActiveManipulator(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY)
    {
        // get the current manipulator
        MCore::Array<MCommon::TransformationManipulator*>* transformationManipulators = GetManager()->GetTransformationManipulators();
        const uint32 numGizmos = transformationManipulators->GetLength();

        // init the active manipulator to nullptr
        MCommon::TransformationManipulator* activeManipulator = nullptr;
        float minCamDist = camera->GetFarClipDistance();
        bool activeManipulatorFound = false;

        // iterate over all gizmos and search for the hit one that is closest to the camera
        for (uint32 i = 0; i < numGizmos; ++i)
        {
            // get the current manipulator and check if it exists
            MCommon::TransformationManipulator* currentManipulator = transformationManipulators->GetItem(i);
            if (currentManipulator == nullptr || currentManipulator->GetIsVisible() == false)
            {
                continue;
            }

            // return the manipulator if selection is already locked
            if (currentManipulator->GetSelectionLocked())
            {
                activeManipulator = currentManipulator;
                activeManipulatorFound = true;
            }

            // check if manipulator is hit or if the selection is locked
            if (currentManipulator->Hit(camera, mousePosX, mousePosY))
            {
                // calculate the distance of the camera and the active manipulator
                float distance = (camera->GetPosition() - currentManipulator->GetPosition()).GetLength();
                if (distance < minCamDist && activeManipulatorFound == false)
                {
                    minCamDist = distance;
                    activeManipulator = currentManipulator;
                }
            }
            else
            {
                if (currentManipulator->GetSelectionLocked() == false)
                {
                    currentManipulator->SetMode(0);
                }
            }
        }

        // return the manipulator
        return activeManipulator;
    }

    // reinit the transformation manipulators
    void RenderPlugin::ReInitTransformationManipulators()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        const RenderOptions::ManipulatorMode mode = mRenderOptions.GetManipulatorMode();

        if (mTranslateManipulator)
        {
            if (actorInstance)
            {
                mTranslateManipulator->Init(actorInstance->GetLocalSpaceTransform().mPosition);
                mTranslateManipulator->SetCallback(new TranslateManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().mPosition));
            }
            mTranslateManipulator->SetIsVisible(actorInstance && mode == RenderOptions::ManipulatorMode::TRANSLATE);
        }

        if (mRotateManipulator)
        {
            if (actorInstance)
            {
                mRotateManipulator->Init(actorInstance->GetLocalSpaceTransform().mPosition);
                mRotateManipulator->SetCallback(new RotateManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().mRotation));
            }
            mRotateManipulator->SetIsVisible(actorInstance && mode == RenderOptions::ManipulatorMode::ROTATE);
        }

        if (mScaleManipulator)
        {
            if (actorInstance)
            {
                mScaleManipulator->Init(actorInstance->GetLocalSpaceTransform().mPosition);
                #ifndef EMFX_SCALE_DISABLED
                    mScaleManipulator->SetCallback(new ScaleManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().mScale));
                #else
                    mScaleManipulator->SetCallback(new ScaleManipulatorCallback(actorInstance, AZ::Vector3::CreateOne()));
                #endif
            }
            mScaleManipulator->SetIsVisible(actorInstance && mode == RenderOptions::ManipulatorMode::SCALE);
        }
    }


    void RenderPlugin::ZoomToJoints(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::Node*>& joints)
    {
        if (!actorInstance || joints.empty())
        {
            return;
        }

        MCore::AABB aabb;
        aabb.Init();

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();

        for (const EMotionFX::Node* joint : joints)
        {
            const AZ::Vector3 jointPosition = pose->GetWorldSpaceTransform(joint->GetNodeIndex()).mPosition;

            aabb.Encapsulate(jointPosition);

            const AZ::u32 childCount = joint->GetNumChildNodes();
            for (AZ::u32 i = 0; i < childCount; ++i)
            {
                EMotionFX::Node* childJoint = skeleton->GetNode(joint->GetChildIndex(i));
                const AZ::Vector3 childPosition = pose->GetWorldSpaceTransform(childJoint->GetNodeIndex()).mPosition;
                aabb.Encapsulate(childPosition);
            }
        }

        if (aabb.CheckIfIsValid())
        {
            aabb.Widen(aabb.CalcRadius());

            bool isFollowModeActive = false;
            for (const RenderViewWidget* viewWidget : m_viewWidgets)
            {
                RenderWidget* current = viewWidget->GetRenderWidget();

                if (viewWidget->GetIsCharacterFollowModeActive())
                {
                    isFollowModeActive = true;
                }

                current->ViewCloseup(aabb, 1.0f);
            }

            if (isFollowModeActive)
            {
                QMessageBox::warning(mDock, "Please disable character follow mode", "Zoom to joints is only working in case character follow mode is disabled.\nPlease disable character follow mode in the render view menu: Camera -> Follow Mode", QMessageBox::Ok);
            }
        }
    }

    void RenderPlugin::OnActorReady([[maybe_unused]] EMotionFX::Actor* actor)
    {
        m_reinitRequested = true;
    }

    // try to locate the helper actor for a given instance
    RenderPlugin::EMStudioRenderActor* RenderPlugin::FindEMStudioActor(EMotionFX::ActorInstance* actorInstance, bool doubleCheckInstance)
    {
        // get the number of emstudio actors and iterate through them
        const uint32 numEMStudioRenderActors = mActors.GetLength();
        for (uint32 i = 0; i < numEMStudioRenderActors; ++i)
        {
            EMStudioRenderActor* EMStudioRenderActor = mActors[i];

            // is the parent actor of the instance the same as the one in the emstudio actor?
            if (EMStudioRenderActor->mActor == actorInstance->GetActor())
            {
                // double check if the actor instance is in the actor instance array inside the emstudio actor
                if (doubleCheckInstance)
                {
                    // now double check if the actor instance really is in the array of instances of this emstudio actor
                    const uint32 numActorInstances = EMStudioRenderActor->mActorInstances.GetLength();
                    for (uint32 a = 0; a < numActorInstances; ++a)
                    {
                        if (EMStudioRenderActor->mActorInstances[a] == actorInstance)
                        {
                            return EMStudioRenderActor;
                        }
                    }
                }
                else
                {
                    return EMStudioRenderActor;
                }
            }
        }

        return nullptr;
    }


    // try to locate the helper actor for a given one
    RenderPlugin::EMStudioRenderActor* RenderPlugin::FindEMStudioActor(EMotionFX::Actor* actor)
    {
        if (!actor)
        {
            return nullptr;
        }

        const uint32 numEMStudioRenderActors = mActors.GetLength();
        for (uint32 i = 0; i < numEMStudioRenderActors; ++i)
        {
            EMStudioRenderActor* EMStudioRenderActor = mActors[i];

            if (EMStudioRenderActor->mActor == actor)
            {
                return EMStudioRenderActor;
            }
        }

        return nullptr;
    }


    // get the index of the given emstudio actor
    uint32 RenderPlugin::FindEMStudioActorIndex(EMStudioRenderActor* EMStudioRenderActor)
    {
        // get the number of emstudio actors and iterate through them
        const uint32 numEMStudioRenderActors = mActors.GetLength();
        for (uint32 i = 0; i < numEMStudioRenderActors; ++i)
        {
            // compare the two emstudio actors and return the current index in case of success
            if (EMStudioRenderActor == mActors[i])
            {
                return i;
            }
        }

        // the emstudio actor has not been found
        return MCORE_INVALIDINDEX32;
    }


    void RegisterRenderPluginLayouts(RenderPlugin* renderPlugin)
    {
        renderPlugin->AddLayout(new SingleRenderWidget());
        renderPlugin->AddLayout(new VerticalDoubleRenderWidget());
        renderPlugin->AddLayout(new HorizontalDoubleRenderWidget());
        renderPlugin->AddLayout(new TripleBigTopRenderWidget());
        renderPlugin->AddLayout(new QuadrupleRenderWidget());
    }


    // add the given render actor to the array
    void RenderPlugin::AddEMStudioActor(EMStudioRenderActor* emstudioActor)
    {
        // add the actor to the list and return success
        mActors.Add(emstudioActor);
    }


    void RenderPlugin::ReInit(bool resetViewCloseup)
    {
        if (!mRenderUtil)
        {
            return;
        }

        // 1. Create new emstudio actors
        uint32 numActors = EMotionFX::GetActorManager().GetNumActors();
        for (uint32 i = 0; i < numActors; ++i)
        {
            // get the current actor and the number of clones
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            if (actor->GetIsOwnedByRuntime() || !actor->IsReady())
            {
                continue;
            }

            // try to find the corresponding emstudio actor
            EMStudioRenderActor* emstudioActor = FindEMStudioActor(actor);

            // in case there is no emstudio actor yet create one
            if (emstudioActor == nullptr)
            {
                CreateEMStudioActor(actor);
            }
        }

        // 2. Remove invalid, not ready or unused emstudio actors
        for (uint32 i = 0; i < mActors.GetLength(); ++i)
        {
            EMStudioRenderActor* emstudioActor = mActors[i];
            EMotionFX::Actor* actor = emstudioActor->mActor;

            bool found = false;
            for (uint32 j = 0; j < numActors; ++j)
            {
                EMotionFX::Actor* curActor = EMotionFX::GetActorManager().GetActor(j);
                if (actor == curActor)
                {
                    found = true;
                }
            }

            // At this point the render actor could point to an already deleted actor.
            // In case the actor got deleted we might get an unexpected flag as result.
            if (!found || (found && actor->GetIsOwnedByRuntime()) || (!actor->IsReady()))
            {
                DestroyEMStudioActor(actor);
            }
        }

        // 3. Relink the actor instances with the emstudio actors
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().GetActorInstance(i);
            EMotionFX::Actor*           actor           = actorInstance->GetActor();
            EMStudioRenderActor*        emstudioActor   = FindEMStudioActor(actorInstance, false);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (!emstudioActor)
            {
                for (uint32 j = 0; j < mActors.GetLength(); ++j)
                {
                    EMStudioRenderActor* currentEMStudioActor = mActors[j];
                    if (actor == currentEMStudioActor->mActor)
                    {
                        emstudioActor = currentEMStudioActor;
                        break;
                    }
                }
            }

            if (emstudioActor)
            {
                // set the GL actor
                actorInstance->SetCustomData(emstudioActor->mRenderActor);

                // add the actor instance to the emstudio actor instances in case it is not in yet
                if (emstudioActor->mActorInstances.Find(actorInstance) == MCORE_INVALIDINDEX32)
                {
                    emstudioActor->mActorInstances.Add(actorInstance);
                }
            }
        }

        // 4. Unlink invalid actor instances from the emstudio actors
        for (uint32 i = 0; i < mActors.GetLength(); ++i)
        {
            EMStudioRenderActor* emstudioActor = mActors[i];

            for (uint32 j = 0; j < emstudioActor->mActorInstances.GetLength();)
            {
                EMotionFX::ActorInstance* emstudioActorInstance = emstudioActor->mActorInstances[j];
                bool found = false;

                for (uint32 k = 0; k < numActorInstances; ++k)
                {
                    if (emstudioActorInstance == EMotionFX::GetActorManager().GetActorInstance(k))
                    {
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    emstudioActor->mActorInstances.Remove(j);
                }
                else
                {
                    j++;
                }
            }
        }

        mFirstFrameAfterReInit = true;
        m_reinitRequested = false;

        // zoom the camera to the available character only in case we're dealing with a single instance
        if (resetViewCloseup && numActorInstances == 1)
        {
            ViewCloseup(false);
        }

        ReInitTransformationManipulators();
    }


    //--------------------------------------------------------------------------------------------------------
    // class EMStudioGLActor
    //--------------------------------------------------------------------------------------------------------

    // constructor
    RenderPlugin::EMStudioRenderActor::EMStudioRenderActor(EMotionFX::Actor* actor, RenderGL::GLActor* renderActor)
    {
        mRenderActor                = renderActor;
        mActor                      = actor;
        mNormalsScaleMultiplier     = 1.0f;
        mCharacterHeight            = 0.0f;
        mOffsetFromTrajectoryNode   = 0.0f;
        mMustCalcNormalScale        = true;

        // extract the bones from the actor and add it to the array
        actor->ExtractBoneList(0, &mBoneList);

        CalculateNormalScaleMultiplier();
    }


    // destructor
    RenderPlugin::EMStudioRenderActor::~EMStudioRenderActor()
    {
        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = mActorInstances.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = mActorInstances[i];

            // only delete the actor instance in case it is still inside the actor manager
            // in case it is not present there anymore this means an undo command has already deleted it
            if (EMotionFX::GetActorManager().FindActorInstanceIndex(actorInstance) != MCORE_INVALIDINDEX32)
            {
                //actorInstance->Destroy();
            }
            // in case the actor instance is not valid anymore make sure to unselect the instance to avoid bad pointers
            else
            {
                CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
                selection.RemoveActorInstance(actorInstance);
            }
        }

        // only delete the actor in case it is still inside the actor manager
        // in case it is not present there anymore this means an undo command has already deleted it
        if (EMotionFX::GetActorManager().FindActorIndex(mActor) != MCORE_INVALIDINDEX32)
        {
            //mActor->Destroy();
        }
        // in case the actor is not valid anymore make sure to unselect it to avoid bad pointers
        else
        {
            CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            selection.RemoveActor(mActor);
        }

        // get rid of the OpenGL actor
        if (mRenderActor)
        {
            mRenderActor->Destroy();
        }
    }


    void RenderPlugin::EMStudioRenderActor::CalculateNormalScaleMultiplier()
    {
        // calculate the max extent of the character
        EMotionFX::ActorInstance* actorInstance = EMotionFX::ActorInstance::Create(mActor);
        actorInstance->UpdateMeshDeformers(0.0f, true);

        MCore::AABB aabb;
        actorInstance->CalcMeshBasedAABB(0, &aabb);

        if (aabb.CheckIfIsValid() == false)
        {
            actorInstance->CalcNodeOBBBasedAABB(&aabb);
        }

        if (aabb.CheckIfIsValid() == false)
        {
            actorInstance->CalcNodeBasedAABB(&aabb);
        }

        mCharacterHeight = aabb.CalcHeight();
        mOffsetFromTrajectoryNode = aabb.GetMin().GetY() + (mCharacterHeight * 0.5f);

        actorInstance->Destroy();

        // scale the normals down to 1% of the character size, that looks pretty nice on all models
        mNormalsScaleMultiplier = aabb.CalcRadius() * 0.01f;
    }


    // zoom to characters
    void RenderPlugin::ViewCloseup(bool selectedInstancesOnly, RenderWidget* renderWidget, float flightTime)
    {
        const MCore::AABB sceneAABB = GetSceneAABB(selectedInstancesOnly);

        if (sceneAABB.CheckIfIsValid())
        {
            // in case the given view widget parameter is nullptr apply it on all view widgets
            if (!renderWidget)
            {
                for (RenderViewWidget* viewWidget : m_viewWidgets)
                {
                    RenderWidget* current = viewWidget->GetRenderWidget();
                    current->ViewCloseup(sceneAABB, flightTime);
                }
            }
            // only apply it to the given view widget
            else
            {
                renderWidget->ViewCloseup(sceneAABB, flightTime);
            }
        }
    }

    void RenderPlugin::SetSkipFollowCalcs(bool skipFollowCalcs)
    {
        for (RenderViewWidget* viewWidget : m_viewWidgets)
        {
            RenderWidget* renderWidget = viewWidget->GetRenderWidget();
            renderWidget->SetSkipFollowCalcs(skipFollowCalcs);
        }
    }

    void RenderPlugin::RemoveViewWidget(RenderViewWidget* viewWidget)
    {
        m_viewWidgets.erase(AZStd::remove(m_viewWidgets.begin(), m_viewWidgets.end(), viewWidget), m_viewWidgets.end());
    }

    void RenderPlugin::ClearViewWidgets()
    {
        m_viewWidgets.clear();
    }

    void RenderPlugin::Reflect(AZ::ReflectContext* context)
    {
        RenderOptions::Reflect(context);
    }

    bool RenderPlugin::Init()
    {
        // load the cursors
        QDir dataDir{ QString(MysticQt::GetDataDir().c_str()) };
        mZoomInCursor       = new QCursor(QPixmap(dataDir.filePath("Images/Rendering/ZoomInCursor.png")).scaled(32, 32));
        mZoomOutCursor      = new QCursor(QPixmap(dataDir.filePath("Images/Rendering/ZoomOutCursor.png")).scaled(32, 32));

        mCurrentSelection   = &GetCommandManager()->GetCurrentSelection();

        connect(mDock, &QDockWidget::visibilityChanged, this, &RenderPlugin::VisibilityChanged);

        // add the available render template layouts
        RegisterRenderPluginLayouts(this);

        // create the inner widget which contains the base layout
        mInnerWidget = new QWidget();
        mDock->setWidget(mInnerWidget);

        // the base layout contains the render layout templates on the left and the render views on the right
        mBaseLayout = new QHBoxLayout(mInnerWidget);
        mBaseLayout->setContentsMargins(0, 2, 2, 2);
        mBaseLayout->setSpacing(0);

        SetSelectionMode();

        // create and register the command callbacks only (only execute this code once for all plugins)
        mUpdateRenderActorsCallback     = new UpdateRenderActorsCallback(false);
        mReInitRenderActorsCallback     = new ReInitRenderActorsCallback(false);
        mCreateActorInstanceCallback    = new CreateActorInstanceCallback(false);
        mRemoveActorInstanceCallback    = new RemoveActorInstanceCallback(false);
        mSelectCallback                 = new SelectCallback(false);
        mUnselectCallback               = new UnselectCallback(false);
        mClearSelectionCallback         = new ClearSelectionCallback(false);
        mResetToBindPoseCallback        = new CommandResetToBindPoseCallback(false);
        mAdjustActorInstanceCallback    = new AdjustActorInstanceCallback(false);
        GetCommandManager()->RegisterCommandCallback("UpdateRenderActors", mUpdateRenderActorsCallback);
        GetCommandManager()->RegisterCommandCallback("ReInitRenderActors", mReInitRenderActorsCallback);
        GetCommandManager()->RegisterCommandCallback("CreateActorInstance", mCreateActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", mRemoveActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("ResetToBindPose", mResetToBindPoseCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActorInstance", mAdjustActorInstanceCallback);

        // initialize the gizmos
        mTranslateManipulator   = (MCommon::TranslateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::TranslateManipulator(70.0f, false));
        mScaleManipulator       = (MCommon::ScaleManipulator*)GetManager()->AddTransformationManipulator(new MCommon::ScaleManipulator(70.0f, false));
        mRotateManipulator      = (MCommon::RotateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::RotateManipulator(70.0f, false));

        // Load the render options and set the last used layout.
        LoadRenderOptions();
        LayoutButtonPressed(mRenderOptions.GetLastUsedLayout().c_str());

        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();
        return true;
    }


    void RenderPlugin::SaveRenderOptions()
    {
        AZStd::string renderOptionsFilename(GetManager()->GetAppDataFolder());
        renderOptionsFilename += "EMStudioRenderOptions.cfg";
        QSettings settings(renderOptionsFilename.c_str(), QSettings::IniFormat, this);

        // save the general render options
        mRenderOptions.Save(&settings);

        AZStd::string groupName;
        if (m_currentLayout)
        {
            const size_t numRenderViews = m_viewWidgets.size();
            for (size_t i = 0; i < numRenderViews; ++i)
            {
                RenderViewWidget* renderView = m_viewWidgets[i];

                groupName = AZStd::string::format("%s_%zu", m_currentLayout->GetName(), i);

                settings.beginGroup(groupName.c_str());
                renderView->SaveOptions(&settings);
                settings.endGroup();
            }
        }
    }


    void RenderPlugin::LoadRenderOptions()
    {
        AZStd::string renderOptionsFilename(GetManager()->GetAppDataFolder());
        renderOptionsFilename += "EMStudioRenderOptions.cfg";
        QSettings settings(renderOptionsFilename.c_str(), QSettings::IniFormat, this);
        mRenderOptions = RenderOptions::Load(&settings);

        AZStd::string groupName;
        if (m_currentLayout)
        {
            const size_t numRenderViews = m_viewWidgets.size();
            for (size_t i = 0; i < numRenderViews; ++i)
            {
                RenderViewWidget* renderView = m_viewWidgets[i];

                groupName = AZStd::string::format("%s_%zu", m_currentLayout->GetName(), i);

                settings.beginGroup(groupName.c_str());
                renderView->LoadOptions(&settings);
                settings.endGroup();
            }
        }

        //SetAspiredRenderingFPS(mRenderOptions.mAspiredRenderFPS);
        SetManipulatorMode(mRenderOptions.GetManipulatorMode());
    }

    void RenderPlugin::SetManipulatorMode(RenderOptions::ManipulatorMode mode)
    {
        mRenderOptions.SetManipulatorMode(mode);

        for (RenderViewWidget* viewWidget : m_viewWidgets)
        {
            viewWidget->SetManipulatorMode(mode);
        }

        ReInitTransformationManipulators();
    }

    void RenderPlugin::VisibilityChanged(bool visible)
    {
        mIsVisible = visible;
    }

    void RenderPlugin::UpdateActorInstances(float timePassedInSeconds)
    {
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            // Ignore actors not owned by the tool.
            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            RenderGL::GLActor* renderActor = static_cast<RenderGL::GLActor*>(actorInstance->GetCustomData());
            if (!renderActor)
            {
                continue;
            }

            UpdateActorInstance(actorInstance, timePassedInSeconds);
        }
    }


    // handle timer event
    void RenderPlugin::ProcessFrame(float timePassedInSeconds)
    {
        // skip rendering in case we want to avoid updating any 3d views
        if (GetManager()->GetAvoidRendering() || mIsVisible == false)
        {
            return;
        }

        if (m_reinitRequested)
        {
            ReInit(/*resetViewCloseup=*/true);
        }

        // update EMotion FX, but don't render
        UpdateActorInstances(timePassedInSeconds);

        for (RenderViewWidget* viewWidget : m_viewWidgets)
        {
            RenderWidget* renderWidget = viewWidget->GetRenderWidget();

            if (!mFirstFrameAfterReInit)
            {
                renderWidget->GetCamera()->Update(timePassedInSeconds);
            }

            if (mFirstFrameAfterReInit)
            {
                mFirstFrameAfterReInit = false;
            }

            // redraw
            renderWidget->Update();
        }
    }


    // get the AABB containing all actor instances in the scene
    MCore::AABB RenderPlugin::GetSceneAABB(bool selectedInstancesOnly)
    {
        MCore::AABB finalAABB;
        CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        if (mUpdateCallback)
        {
            mUpdateCallback->SetEnableRendering(false);
        }

        // update EMotion FX, but don't render
        EMotionFX::GetEMotionFX().Update(0.0f);

        if (mUpdateCallback)
        {
            mUpdateCallback->SetEnableRendering(true);
        }

        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance and update its transformations and meshes
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            // only take the selected instances into account if wanted
            if (selectedInstancesOnly)
            {
                if (selection.CheckIfHasActorInstance(actorInstance) == false)
                {
                    continue;
                }
            }

            // get the mesh based AABB
            MCore::AABB aabb;
            actorInstance->CalcMeshBasedAABB(0, &aabb);

            // get the node based AABB
            if (aabb.CheckIfIsValid() == false)
            {
                actorInstance->CalcNodeBasedAABB(&aabb);
            }

            // make sure the actor instance is covered in our global bounding box
            finalAABB.Encapsulate(aabb);
        }

        return finalAABB;
    }


    void RenderPlugin::OnAfterLoadProject()
    {
        ViewCloseup(false);
    }

    void RenderPlugin::OnAfterLoadActors()
    {
        ViewCloseup(false);
    }

    RenderPlugin::Layout* RenderPlugin::FindLayoutByName(const AZStd::string& layoutName) const
    {
        for (Layout* layout : m_layouts)
        {
            if (AzFramework::StringFunc::Equal(layoutName.c_str(), layout->GetName(), false /* no case */))
            {
                return layout;
            }
        }

        // return the first layout in case it wasn't found
        if (!m_layouts.empty())
        {
            return m_layouts[0];
        }

        return nullptr;
    }


    void RenderPlugin::LayoutButtonPressed(const QString& text)
    {
        AZStd::string pressedButtonText = FromQtString(text);
        Layout* layout = FindLayoutByName(pressedButtonText);
        if (layout == nullptr)
        {
            return;
        }

        // save the current settings and disable rendering
        mRenderOptions.SetLastUsedLayout(layout->GetName());
        ClearViewWidgets();
        VisibilityChanged(false);

        m_currentLayout = layout;

        QWidget* oldLayoutWidget = mRenderLayoutWidget;
        QWidget* newLayoutWidget = layout->Create(this, mInnerWidget);

        // delete the old render layout after we created the new one, so we can keep the old resources
        // this only removes it from the layout
        mBaseLayout->removeWidget(oldLayoutWidget);

        mRenderLayoutWidget = newLayoutWidget;

        // create thw new one and add it to the base layout
        mBaseLayout->addWidget(mRenderLayoutWidget);
        mRenderLayoutWidget->update();
        mBaseLayout->update();
        mRenderLayoutWidget->show();

        LoadRenderOptions();
        ViewCloseup(false, nullptr, 0.0f);

        // get rid of the layout widget
        if (oldLayoutWidget)
        {
            oldLayoutWidget->hide();
            oldLayoutWidget->deleteLater();
        }

        VisibilityChanged(true);
    }


    RenderViewWidget* RenderPlugin::CreateViewWidget(QWidget* parent)
    {
        RenderViewWidget* viewWidget = new RenderViewWidget(this, parent);
        m_viewWidgets.emplace_back(viewWidget);
        return viewWidget;
    }


    // find the trajectory path for a given actor instance
    MCommon::RenderUtil::TrajectoryTracePath* RenderPlugin::FindTracePath(EMotionFX::ActorInstance* actorInstance)
    {
        for (MCommon::RenderUtil::TrajectoryTracePath* trajectoryPath : m_trajectoryTracePaths)
        {
            if (trajectoryPath->mActorInstance == actorInstance)
            {
                return trajectoryPath;
            }
        }

        // we haven't created a path for the given actor instance yet, do so
        MCommon::RenderUtil::TrajectoryTracePath* tracePath = new MCommon::RenderUtil::TrajectoryTracePath();

        tracePath->mActorInstance = actorInstance;
        tracePath->mTraceParticles.Reserve(512);

        m_trajectoryTracePaths.emplace_back(tracePath);
        return tracePath;
    }


    // reset the trajectory paths for all selected actor instances
    void RenderPlugin::ResetSelectedTrajectoryPaths()
    {
        // get the current selection
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();

        // iterate through the actor instances and reset their trajectory path
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            // get the actor instance and find the corresponding trajectory path
            EMotionFX::ActorInstance*                   actorInstance   = selectionList.GetActorInstance(i);
            MCommon::RenderUtil::TrajectoryTracePath*   trajectoryPath  = FindTracePath(actorInstance);

            // reset the trajectory path
            MCommon::RenderUtil::ResetTrajectoryPath(trajectoryPath);
        }
    }


    // update a visible character
    void RenderPlugin::UpdateActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        // find the corresponding trajectory trace path for the given actor instance
        MCommon::RenderUtil::TrajectoryTracePath* trajectoryPath = FindTracePath(actorInstance);
        if (trajectoryPath)
        {
            const EMotionFX::Actor* actor = actorInstance->GetActor();
            const EMotionFX::Node* motionExtractionNode = actor->GetMotionExtractionNode();
            if (motionExtractionNode)
            {
                const EMotionFX::Transform& worldTM = actorInstance->GetWorldSpaceTransform();

                bool distanceTraveledEnough = false;
                if (trajectoryPath->mTraceParticles.GetIsEmpty())
                {
                    distanceTraveledEnough = true;
                }
                else
                {
                    const uint32 numParticles = trajectoryPath->mTraceParticles.GetLength();
                    const EMotionFX::Transform& oldWorldTM = trajectoryPath->mTraceParticles[numParticles - 1].mWorldTM;

                    const AZ::Vector3& oldPos = oldWorldTM.mPosition;
                    const AZ::Quaternion oldRot = oldWorldTM.mRotation.GetNormalized();
                    const AZ::Quaternion rotation = worldTM.mRotation.GetNormalized();

                    const AZ::Vector3 deltaPos = worldTM.mPosition - oldPos;
                    const float deltaRot = MCore::Math::Abs(rotation.Dot(oldRot));
                    if (MCore::SafeLength(deltaPos) > 0.0001f || deltaRot < 0.99f)
                    {
                        distanceTraveledEnough = true;
                    }
                }

                // add the time delta to the time passed since the last add
                trajectoryPath->mTimePassed += timePassedInSeconds;

                const uint32 particleSampleRate = 30;
                if (trajectoryPath->mTimePassed >= (1.0f / particleSampleRate) && distanceTraveledEnough)
                {
                    // create the particle, fill its data and add it to the trajectory trace path
                    MCommon::RenderUtil::TrajectoryPathParticle trajectoryParticle;
                    trajectoryParticle.mWorldTM = worldTM;
                    trajectoryPath->mTraceParticles.Add(trajectoryParticle);

                    // reset the time passed as we just added a new particle
                    trajectoryPath->mTimePassed = 0.0f;
                }
            }

            // make sure we don't have too many items in our array
            if (trajectoryPath->mTraceParticles.GetLength() > 50)
            {
                trajectoryPath->mTraceParticles.RemoveFirst();
            }
        }
    }


    // update a visible character
    void RenderPlugin::RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);
        RenderPlugin::EMStudioRenderActor* emstudioActor = FindEMStudioActor(actorInstance);
        if (emstudioActor == nullptr)
        {
            return;
        }

        // renderUtil options
        MCommon::RenderUtil* renderUtil = GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // get the active widget & it's rendering options
        RenderViewWidget*   widget          = GetActiveViewWidget();
        RenderOptions*      renderOptions   = GetRenderOptions();

        const AZStd::unordered_set<AZ::u32>& visibleJointIndices = GetManager()->GetVisibleJointIndices();
        const AZStd::unordered_set<AZ::u32>& selectedJointIndices = GetManager()->GetSelectedJointIndices();

        // render the AABBs
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_AABB))
        {
            MCommon::RenderUtil::AABBRenderSettings settings;
            settings.mNodeBasedColor          = renderOptions->GetNodeAABBColor();
            settings.mStaticBasedColor        = renderOptions->GetStaticAABBColor();
            settings.mMeshBasedColor          = renderOptions->GetMeshAABBColor();
            settings.mCollisionMeshBasedColor = renderOptions->GetCollisionMeshAABBColor();

            renderUtil->RenderAABBs(actorInstance, settings);
        }

        if (widget->GetRenderFlag(RenderViewWidget::RENDER_OBB))
        {
            renderUtil->RenderOBBs(actorInstance, &visibleJointIndices, &selectedJointIndices, renderOptions->GetOBBsColor(), renderOptions->GetSelectedObjectColor());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_LINESKELETON))
        {
            const MCommon::Camera* camera = widget->GetRenderWidget()->GetCamera();
            const AZ::Vector3& cameraPos = camera->GetPosition();

            MCore::AABB aabb;
            actorInstance->CalcNodeBasedAABB(&aabb);
            const AZ::Vector3 aabbMid = aabb.CalcMiddle();
            const float aabbRadius = aabb.CalcRadius();
            const float camDistance = fabs((cameraPos - aabbMid).GetLength());

            // Avoid rendering too big joint spheres when zooming in onto a joint.
            float scaleMultiplier = 1.0f;
            if (camDistance < aabbRadius)
            {
                scaleMultiplier = camDistance / aabbRadius;
            }

            // Scale the joint spheres based on the character's extents, to avoid really large joint spheres
            // on small characters and too small spheres on large characters.
            static const float baseRadius = 0.005f;
            const float jointSphereRadius = aabb.CalcRadius() * scaleMultiplier * baseRadius;

            renderUtil->RenderSimpleSkeleton(actorInstance, &visibleJointIndices, &selectedJointIndices,
                renderOptions->GetLineSkeletonColor(), renderOptions->GetSelectedObjectColor(), jointSphereRadius);
        }

        bool cullingEnabled = renderUtil->GetCullingEnabled();
        bool lightingEnabled = renderUtil->GetLightingEnabled();
        renderUtil->EnableCulling(false); // disable culling
        renderUtil->EnableLighting(false); // disable lighting
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_SKELETON))
        {
            renderUtil->RenderSkeleton(actorInstance, emstudioActor->mBoneList, &visibleJointIndices, &selectedJointIndices, renderOptions->GetSkeletonColor(), renderOptions->GetSelectedObjectColor());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODEORIENTATION))
        {
            renderUtil->RenderNodeOrientations(actorInstance, emstudioActor->mBoneList, &visibleJointIndices, &selectedJointIndices, emstudioActor->mNormalsScaleMultiplier * renderOptions->GetNodeOrientationScale(), renderOptions->GetScaleBonesOnLength());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_ACTORBINDPOSE))
        {
            renderUtil->RenderBindPose(actorInstance);
        }

        // render motion extraction debug info
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_MOTIONEXTRACTION))
        {
            // render an arrow for the trajectory
            renderUtil->RenderTrajectoryPath(FindTracePath(actorInstance), renderOptions->GetTrajectoryArrowInnerColor(), emstudioActor->mCharacterHeight * 0.05f);
        }
        renderUtil->EnableCulling(cullingEnabled); // reset to the old state
        renderUtil->EnableLighting(lightingEnabled);

        const bool renderVertexNormals  = widget->GetRenderFlag(RenderViewWidget::RENDER_VERTEXNORMALS);
        const bool renderFaceNormals    = widget->GetRenderFlag(RenderViewWidget::RENDER_FACENORMALS);
        const bool renderTangents       = widget->GetRenderFlag(RenderViewWidget::RENDER_TANGENTS);
        const bool renderWireframe      = widget->GetRenderFlag(RenderViewWidget::RENDER_WIREFRAME);
        const bool renderCollisionMeshes= widget->GetRenderFlag(RenderViewWidget::RENDER_COLLISIONMESHES);

        if (renderVertexNormals || renderFaceNormals || renderTangents || renderWireframe || renderCollisionMeshes)
        {
            // iterate through all enabled nodes
            const EMotionFX::Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();

            const uint32 geomLODLevel   = actorInstance->GetLODLevel();
            const uint32 numEnabled     = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numEnabled; ++i)
            {
                EMotionFX::Node*    node      = emstudioActor->mActor->GetSkeleton()->GetNode(actorInstance->GetEnabledNode(i));
                const AZ::u32       nodeIndex = node->GetNodeIndex();
                EMotionFX::Mesh*    mesh      = emstudioActor->mActor->GetMesh(geomLODLevel, nodeIndex);

                renderUtil->ResetCurrentMesh();

                if (!mesh)
                {
                    continue;
                }

                const AZ::Transform worldTM = pose->GetMeshNodeWorldSpaceTransform(geomLODLevel, nodeIndex).ToAZTransform();

                if (!mesh->GetIsCollisionMesh())
                {
                    renderUtil->RenderNormals(mesh, worldTM, renderVertexNormals, renderFaceNormals, renderOptions->GetVertexNormalsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetFaceNormalsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetVertexNormalsColor(), renderOptions->GetFaceNormalsColor());
                    if (renderTangents)
                    {
                        renderUtil->RenderTangents(mesh, worldTM, renderOptions->GetTangentsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetTangentsColor(), renderOptions->GetMirroredBitangentsColor(), renderOptions->GetBitangentsColor());
                    }
                    if (renderWireframe)
                    {
                        renderUtil->RenderWireframe(mesh, worldTM, renderOptions->GetWireframeColor(), false, emstudioActor->mNormalsScaleMultiplier);
                    }
                }
                else
                if (renderCollisionMeshes)
                {
                    renderUtil->RenderWireframe(mesh, worldTM, renderOptions->GetCollisionMeshColor(), false, emstudioActor->mNormalsScaleMultiplier);
                }
            }
        }

        // render the selection
        if (renderOptions->GetRenderSelectionBox() && EMotionFX::GetActorManager().GetNumActorInstances() != 1 && GetCurrentSelection()->CheckIfHasActorInstance(actorInstance))
        {
            MCore::AABB aabb = actorInstance->GetAABB();
            aabb.Widen(aabb.CalcRadius() * 0.005f);
            renderUtil->RenderSelection(aabb, renderOptions->GetSelectionColor());
        }

        // render node names
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODENAMES))
        {
            MCommon::Camera*    camera          = widget->GetRenderWidget()->GetCamera();
            const uint32        screenWidth     = widget->GetRenderWidget()->GetScreenWidth();
            const uint32        screenHeight    = widget->GetRenderWidget()->GetScreenHeight();

            renderUtil->RenderNodeNames(actorInstance, camera, screenWidth, screenHeight, renderOptions->GetNodeNameColor(), renderOptions->GetSelectedObjectColor(), visibleJointIndices, selectedJointIndices);
        }
    }


    void RenderPlugin::ResetCameras()
    {
        for (RenderViewWidget* viewWidget : m_viewWidgets)
        {
            viewWidget->OnResetCamera();
        }
    }
} // namespace EMStudio
