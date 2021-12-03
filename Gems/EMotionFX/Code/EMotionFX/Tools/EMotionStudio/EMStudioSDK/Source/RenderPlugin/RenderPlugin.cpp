/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_isVisible                      = true;
        m_renderUtil                     = nullptr;
        m_updateCallback                 = nullptr;

        m_updateRenderActorsCallback     = nullptr;
        m_reInitRenderActorsCallback     = nullptr;
        m_createActorInstanceCallback    = nullptr;
        m_removeActorInstanceCallback    = nullptr;
        m_selectCallback                 = nullptr;
        m_unselectCallback               = nullptr;
        m_clearSelectionCallback         = nullptr;
        m_resetToBindPoseCallback        = nullptr;
        m_adjustActorInstanceCallback    = nullptr;

        m_zoomInCursor                   = nullptr;
        m_zoomOutCursor                  = nullptr;

        m_baseLayout                     = nullptr;
        m_renderLayoutWidget             = nullptr;
        m_activeViewWidget               = nullptr;
        m_currentSelection               = nullptr;
        m_currentLayout                 = nullptr;
        m_focusViewWidget                = nullptr;
        m_firstFrameAfterReInit          = false;

        m_translateManipulator           = nullptr;
        m_rotateManipulator              = nullptr;
        m_scaleManipulator               = nullptr;

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
        GetManager()->RemoveTransformationManipulator(m_translateManipulator);
        GetManager()->RemoveTransformationManipulator(m_rotateManipulator);
        GetManager()->RemoveTransformationManipulator(m_scaleManipulator);

        delete m_translateManipulator;
        delete m_rotateManipulator;
        delete m_scaleManipulator;

        // get rid of the cursors
        delete m_zoomInCursor;
        delete m_zoomOutCursor;

        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(m_updateRenderActorsCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_reInitRenderActorsCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_createActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_selectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_unselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_clearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_resetToBindPoseCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_adjustActorInstanceCallback, false);

        delete m_updateRenderActorsCallback;
        delete m_reInitRenderActorsCallback;
        delete m_createActorInstanceCallback;
        delete m_removeActorInstanceCallback;
        delete m_selectCallback;
        delete m_unselectCallback;
        delete m_clearSelectionCallback;
        delete m_resetToBindPoseCallback;
        delete m_adjustActorInstanceCallback;

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
        for (EMStudioRenderActor* actor : m_actors)
        {
            if (actor)
            {
                delete actor;
            }
        }
        m_actors.clear();
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
        const size_t index = FindEMStudioActorIndex(emstudioActor);
        MCORE_ASSERT(index != InvalidIndex);

        // get rid of the emstudio actor
        delete emstudioActor;
        m_actors.erase(AZStd::next(begin(m_actors), index));
        return true;
    }


    // finds the manipulator the is currently hit by the mouse within the current camera
    MCommon::TransformationManipulator* RenderPlugin::GetActiveManipulator(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY)
    {
        // get the current manipulator
        AZStd::vector<MCommon::TransformationManipulator*>* transformationManipulators = GetManager()->GetTransformationManipulators();

        // init the active manipulator to nullptr
        MCommon::TransformationManipulator* activeManipulator = nullptr;
        float minCamDist = camera->GetFarClipDistance();
        bool activeManipulatorFound = false;

        // iterate over all gizmos and search for the hit one that is closest to the camera
        for (MCommon::TransformationManipulator* currentManipulator : *transformationManipulators)
        {
            // get the current manipulator and check if it exists
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
        const RenderOptions::ManipulatorMode mode = m_renderOptions.GetManipulatorMode();

        if (m_translateManipulator)
        {
            if (actorInstance)
            {
                m_translateManipulator->Init(actorInstance->GetLocalSpaceTransform().m_position);
                m_translateManipulator->SetCallback(new TranslateManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().m_position));
            }
            m_translateManipulator->SetIsVisible(actorInstance && mode == RenderOptions::ManipulatorMode::TRANSLATE);
        }

        if (m_rotateManipulator)
        {
            if (actorInstance)
            {
                m_rotateManipulator->Init(actorInstance->GetLocalSpaceTransform().m_position);
                m_rotateManipulator->SetCallback(new RotateManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().m_rotation));
            }
            m_rotateManipulator->SetIsVisible(actorInstance && mode == RenderOptions::ManipulatorMode::ROTATE);
        }

        if (m_scaleManipulator)
        {
            if (actorInstance)
            {
                m_scaleManipulator->Init(actorInstance->GetLocalSpaceTransform().m_position);
                #ifndef EMFX_SCALE_DISABLED
                    m_scaleManipulator->SetCallback(new ScaleManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().m_scale));
                #else
                    m_scaleManipulator->SetCallback(new ScaleManipulatorCallback(actorInstance, AZ::Vector3::CreateOne()));
                #endif
            }
            m_scaleManipulator->SetIsVisible(actorInstance && mode == RenderOptions::ManipulatorMode::SCALE);
        }
    }


    void RenderPlugin::ZoomToJoints(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::Node*>& joints)
    {
        if (!actorInstance || joints.empty())
        {
            return;
        }

        AZ::Aabb aabb = AZ::Aabb::CreateNull();

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();

        for (const EMotionFX::Node* joint : joints)
        {
            const AZ::Vector3 jointPosition = pose->GetWorldSpaceTransform(joint->GetNodeIndex()).m_position;
            aabb.AddPoint(jointPosition);

            const size_t childCount = joint->GetNumChildNodes();
            for (size_t i = 0; i < childCount; ++i)
            {
                EMotionFX::Node* childJoint = skeleton->GetNode(joint->GetChildIndex(i));
                const AZ::Vector3 childPosition = pose->GetWorldSpaceTransform(childJoint->GetNodeIndex()).m_position;
                aabb.AddPoint(childPosition);
            }
        }

        if (aabb.IsValid())
        {
            aabb.Expand(AZ::Vector3(aabb.GetExtents().GetLength() * 0.5f));

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
                QMessageBox::warning(m_dock, "Please disable character follow mode", "Zoom to joints is only working in case character follow mode is disabled.\nPlease disable character follow mode in the render view menu: Camera -> Follow Mode", QMessageBox::Ok);
            }
        }
    }

    void RenderPlugin::OnActorReady([[maybe_unused]] EMotionFX::Actor* actor)
    {
        m_reinitRequested = true;
    }

    // try to locate the helper actor for a given instance
    RenderPlugin::EMStudioRenderActor* RenderPlugin::FindEMStudioActor(const EMotionFX::ActorInstance* actorInstance, bool doubleCheckInstance) const
    {
        const auto foundActor = AZStd::find_if(begin(m_actors), end(m_actors), [actorInstance, doubleCheckInstance](const EMStudioRenderActor* renderActor)
        {
            // is the parent actor of the instance the same as the one in the emstudio actor?
            if (renderActor->m_actor == actorInstance->GetActor())
            {
                // double check if the actor instance is in the actor instance array inside the emstudio actor
                if (doubleCheckInstance)
                {
                    // now double check if the actor instance really is in the array of instances of this emstudio actor
                    const auto foundActorInstance = AZStd::find(begin(renderActor->m_actorInstances), end(renderActor->m_actorInstances), actorInstance);
                    return foundActorInstance != end(renderActor->m_actorInstances);
                }
                return true;
            }
            return false;
        });
        return foundActor != end(m_actors) ? *foundActor : nullptr;
    }


    // try to locate the helper actor for a given one
    RenderPlugin::EMStudioRenderActor* RenderPlugin::FindEMStudioActor(const EMotionFX::Actor* actor) const
    {
        if (!actor)
        {
            return nullptr;
        }

        const auto foundActor = AZStd::find_if(begin(m_actors), end(m_actors), [match = actor](const EMStudioRenderActor* actor)
        {
            return actor->m_actor == match;
        });
        return foundActor != end(m_actors) ? *foundActor : nullptr;
    }


    // get the index of the given emstudio actor
    size_t RenderPlugin::FindEMStudioActorIndex(const EMStudioRenderActor* EMStudioRenderActor) const
    {
        const auto foundActor = AZStd::find(begin(m_actors), end(m_actors), EMStudioRenderActor);
        return foundActor != end(m_actors) ? AZStd::distance(begin(m_actors), foundActor) : InvalidIndex;
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
        m_actors.emplace_back(emstudioActor);
    }


    void RenderPlugin::ReInit(bool resetViewCloseup)
    {
        if (!m_renderUtil)
        {
            return;
        }

        // 1. Create new emstudio actors
        size_t numActors = EMotionFX::GetActorManager().GetNumActors();
        for (size_t i = 0; i < numActors; ++i)
        {
            // get the current actor and the number of clones
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            if (!actor->IsReady())
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

        for (size_t i = 0; i < m_actors.size(); ++i)
        {
            EMStudioRenderActor* emstudioActor = m_actors[i];
            EMotionFX::Actor* actor = emstudioActor->m_actor;

            bool found = false;
            for (size_t j = 0; j < numActors; ++j)
            {
                EMotionFX::Actor* curActor = EMotionFX::GetActorManager().GetActor(j);
                if (actor == curActor)
                {
                    found = true;
                }
            }

            // At this point the render actor could point to an already deleted actor.
            // In case the actor got deleted we might get an unexpected flag as result.
            if (!found || (!actor->IsReady()))
            {
                DestroyEMStudioActor(actor);
            }
        }

        // 3. Relink the actor instances with the emstudio actors
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
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
                for (EMStudioRenderActor* currentEMStudioActor : m_actors)
                {
                    if (actor == currentEMStudioActor->m_actor)
                    {
                        emstudioActor = currentEMStudioActor;
                        break;
                    }
                }
            }

            if (emstudioActor)
            {
                // set the GL actor
                actorInstance->SetCustomData(emstudioActor->m_renderActor);

                // add the actor instance to the emstudio actor instances in case it is not in yet
                if (AZStd::find(begin(emstudioActor->m_actorInstances), end(emstudioActor->m_actorInstances), actorInstance) == end(emstudioActor->m_actorInstances))
                {
                    emstudioActor->m_actorInstances.emplace_back(actorInstance);
                }
            }
        }

        // 4. Unlink invalid actor instances from the emstudio actors
        for (EMStudioRenderActor* emstudioActor : m_actors)
        {
            for (size_t j = 0; j < emstudioActor->m_actorInstances.size();)
            {
                EMotionFX::ActorInstance* emstudioActorInstance = emstudioActor->m_actorInstances[j];
                bool found = false;

                for (size_t k = 0; k < numActorInstances; ++k)
                {
                    if (emstudioActorInstance == EMotionFX::GetActorManager().GetActorInstance(k))
                    {
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    emstudioActor->m_actorInstances.erase(AZStd::next(begin(emstudioActor->m_actorInstances), j));
                }
                else
                {
                    j++;
                }
            }
        }

        m_firstFrameAfterReInit = true;
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
        m_renderActor                = renderActor;
        m_actor                      = actor;
        m_normalsScaleMultiplier     = 1.0f;
        m_characterHeight            = 0.0f;
        m_offsetFromTrajectoryNode   = 0.0f;
        m_mustCalcNormalScale        = true;

        // extract the bones from the actor and add it to the array
        actor->ExtractBoneList(0, &m_boneList);

        CalculateNormalScaleMultiplier();
    }


    // destructor
    RenderPlugin::EMStudioRenderActor::~EMStudioRenderActor()
    {
        // get the number of actor instances and iterate through them
        for (EMotionFX::ActorInstance* actorInstance : m_actorInstances)
        {
            // only delete the actor instance in case it is still inside the actor manager
            // in case it is not present there anymore this means an undo command has already deleted it
            if (EMotionFX::GetActorManager().FindActorInstanceIndex(actorInstance) != InvalidIndex)
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
        if (EMotionFX::GetActorManager().FindActorIndex(m_actor) == InvalidIndex)
        {
            // in case the actor is not valid anymore make sure to unselect it to avoid bad pointers
            CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            selection.RemoveActor(m_actor);
        }

        // get rid of the OpenGL actor
        if (m_renderActor)
        {
            m_renderActor->Destroy();
        }
    }


    void RenderPlugin::EMStudioRenderActor::CalculateNormalScaleMultiplier()
    {
        // calculate the max extent of the character
        EMotionFX::ActorInstance* actorInstance = EMotionFX::ActorInstance::Create(m_actor);
        actorInstance->UpdateMeshDeformers(0.0f, true);

        AZ::Aabb aabb;
        actorInstance->CalcMeshBasedAabb(0, &aabb);

        if (!aabb.IsValid())
        {
            actorInstance->CalcNodeBasedAabb(&aabb);
        }

        m_characterHeight = aabb.GetExtents().GetZ();
        m_offsetFromTrajectoryNode = aabb.GetMin().GetY() + (m_characterHeight * 0.5f);

        actorInstance->Destroy();

        // scale the normals down to 1% of the character size, that looks pretty nice on all models
        const float radius = AZ::Vector3(aabb.GetMax() - aabb.GetMin()).GetLength() * 0.5f;
        m_normalsScaleMultiplier = radius * 0.01f;
    }


    // zoom to characters
    void RenderPlugin::ViewCloseup(bool selectedInstancesOnly, RenderWidget* renderWidget, float flightTime)
    {
        const AZ::Aabb sceneAabb = GetSceneAabb(selectedInstancesOnly);
        if (sceneAabb.IsValid())
        {
            // in case the given view widget parameter is nullptr apply it on all view widgets
            if (!renderWidget)
            {
                for (RenderViewWidget* viewWidget : m_viewWidgets)
                {
                    RenderWidget* current = viewWidget->GetRenderWidget();
                    current->ViewCloseup(sceneAabb, flightTime);
                }
            }
            // only apply it to the given view widget
            else
            {
                renderWidget->ViewCloseup(sceneAabb, flightTime);
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
        m_zoomInCursor       = new QCursor(QPixmap(dataDir.filePath("Images/Rendering/ZoomInCursor.png")).scaled(32, 32));
        m_zoomOutCursor      = new QCursor(QPixmap(dataDir.filePath("Images/Rendering/ZoomOutCursor.png")).scaled(32, 32));

        m_currentSelection   = &GetCommandManager()->GetCurrentSelection();

        connect(m_dock, &QDockWidget::visibilityChanged, this, &RenderPlugin::VisibilityChanged);

        // add the available render template layouts
        RegisterRenderPluginLayouts(this);

        // create the inner widget which contains the base layout
        m_innerWidget = new QWidget();
        m_dock->setWidget(m_innerWidget);

        // the base layout contains the render layout templates on the left and the render views on the right
        m_baseLayout = new QHBoxLayout(m_innerWidget);
        m_baseLayout->setContentsMargins(0, 2, 2, 2);
        m_baseLayout->setSpacing(0);

        SetSelectionMode();

        // create and register the command callbacks only (only execute this code once for all plugins)
        m_updateRenderActorsCallback     = new UpdateRenderActorsCallback(false);
        m_reInitRenderActorsCallback     = new ReInitRenderActorsCallback(false);
        m_createActorInstanceCallback    = new CreateActorInstanceCallback(false);
        m_removeActorInstanceCallback    = new RemoveActorInstanceCallback(false);
        m_selectCallback                 = new SelectCallback(false);
        m_unselectCallback               = new UnselectCallback(false);
        m_clearSelectionCallback         = new ClearSelectionCallback(false);
        m_resetToBindPoseCallback        = new CommandResetToBindPoseCallback(false);
        m_adjustActorInstanceCallback    = new AdjustActorInstanceCallback(false);
        GetCommandManager()->RegisterCommandCallback("UpdateRenderActors", m_updateRenderActorsCallback);
        GetCommandManager()->RegisterCommandCallback("ReInitRenderActors", m_reInitRenderActorsCallback);
        GetCommandManager()->RegisterCommandCallback("CreateActorInstance", m_createActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", m_removeActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("Select", m_selectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", m_unselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_clearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("ResetToBindPose", m_resetToBindPoseCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActorInstance", m_adjustActorInstanceCallback);

        // initialize the gizmos
        m_translateManipulator   = (MCommon::TranslateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::TranslateManipulator(70.0f, false));
        m_scaleManipulator       = (MCommon::ScaleManipulator*)GetManager()->AddTransformationManipulator(new MCommon::ScaleManipulator(70.0f, false));
        m_rotateManipulator      = (MCommon::RotateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::RotateManipulator(70.0f, false));

        // Load the render options and set the last used layout.
        LoadRenderOptions();
        LayoutButtonPressed(m_renderOptions.GetLastUsedLayout().c_str());

        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();
        return true;
    }


    void RenderPlugin::SaveRenderOptions()
    {
        AZStd::string renderOptionsFilename(GetManager()->GetAppDataFolder());
        renderOptionsFilename += "EMStudioRenderOptions.cfg";
        QSettings settings(renderOptionsFilename.c_str(), QSettings::IniFormat, this);

        // save the general render options
        m_renderOptions.Save(&settings);

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
        m_renderOptions = RenderOptions::Load(&settings);

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

        SetManipulatorMode(m_renderOptions.GetManipulatorMode());
    }

    void RenderPlugin::SetManipulatorMode(RenderOptions::ManipulatorMode mode)
    {
        m_renderOptions.SetManipulatorMode(mode);

        for (RenderViewWidget* viewWidget : m_viewWidgets)
        {
            viewWidget->SetManipulatorMode(mode);
        }

        ReInitTransformationManipulators();
    }

    void RenderPlugin::VisibilityChanged(bool visible)
    {
        m_isVisible = visible;
    }

    void RenderPlugin::UpdateActorInstances(float timePassedInSeconds)
    {
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
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
        if (GetManager()->GetAvoidRendering() || m_isVisible == false)
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

            if (!m_firstFrameAfterReInit)
            {
                renderWidget->GetCamera()->Update(timePassedInSeconds);
            }

            if (m_firstFrameAfterReInit)
            {
                m_firstFrameAfterReInit = false;
            }

            // redraw
            renderWidget->Update();
        }
    }


    // get the AABB containing all actor instances in the scene
    AZ::Aabb RenderPlugin::GetSceneAabb(bool selectedInstancesOnly)
    {
        AZ::Aabb finalAabb = AZ::Aabb::CreateNull();
        CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        if (m_updateCallback)
        {
            m_updateCallback->SetEnableRendering(false);
        }

        // update EMotion FX, but don't render
        EMotionFX::GetEMotionFX().Update(0.0f);

        if (m_updateCallback)
        {
            m_updateCallback->SetEnableRendering(true);
        }

        // get the number of actor instances and iterate through them
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
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
            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            actorInstance->CalcMeshBasedAabb(0, &aabb);

            // get the node based AABB
            if (!aabb.IsValid())
            {
                actorInstance->CalcNodeBasedAabb(&aabb);
            }

            // make sure the actor instance is covered in our global bounding box
            if (aabb.IsValid())
            {
                finalAabb.AddAabb(aabb);
            }
        }

        if (!finalAabb.IsValid())
        {
            finalAabb.Set(AZ::Vector3(-1.0f, -1.0f, 0.0f), AZ::Vector3(1.0f, 1.0f, 0.0f));
        }

        return finalAabb;
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
        m_renderOptions.SetLastUsedLayout(layout->GetName());
        SaveRenderOptions();
        ClearViewWidgets();
        VisibilityChanged(false);

        m_currentLayout = layout;

        QWidget* oldLayoutWidget = m_renderLayoutWidget;
        QWidget* newLayoutWidget = layout->Create(this, m_innerWidget);

        // delete the old render layout after we created the new one, so we can keep the old resources
        // this only removes it from the layout
        m_baseLayout->removeWidget(oldLayoutWidget);

        m_renderLayoutWidget = newLayoutWidget;

        // create thw new one and add it to the base layout
        m_baseLayout->addWidget(m_renderLayoutWidget);
        m_renderLayoutWidget->update();
        m_baseLayout->update();
        m_renderLayoutWidget->show();

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
            if (trajectoryPath->m_actorInstance == actorInstance)
            {
                return trajectoryPath;
            }
        }

        // we haven't created a path for the given actor instance yet, do so
        MCommon::RenderUtil::TrajectoryTracePath* tracePath = new MCommon::RenderUtil::TrajectoryTracePath();

        tracePath->m_actorInstance = actorInstance;
        tracePath->m_traceParticles.reserve(512);

        m_trajectoryTracePaths.emplace_back(tracePath);
        return tracePath;
    }


    // reset the trajectory paths for all selected actor instances
    void RenderPlugin::ResetSelectedTrajectoryPaths()
    {
        // get the current selection
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();

        // iterate through the actor instances and reset their trajectory path
        for (size_t i = 0; i < numSelectedActorInstances; ++i)
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
                if (trajectoryPath->m_traceParticles.empty())
                {
                    distanceTraveledEnough = true;
                }
                else
                {
                    const size_t numParticles = trajectoryPath->m_traceParticles.size();
                    const EMotionFX::Transform& oldWorldTM = trajectoryPath->m_traceParticles[numParticles - 1].m_worldTm;

                    const AZ::Vector3& oldPos = oldWorldTM.m_position;
                    const AZ::Quaternion oldRot = oldWorldTM.m_rotation.GetNormalized();
                    const AZ::Quaternion rotation = worldTM.m_rotation.GetNormalized();

                    const AZ::Vector3 deltaPos = worldTM.m_position - oldPos;
                    const float deltaRot = MCore::Math::Abs(rotation.Dot(oldRot));
                    if (MCore::SafeLength(deltaPos) > 0.0001f || deltaRot < 0.99f)
                    {
                        distanceTraveledEnough = true;
                    }
                }

                // add the time delta to the time passed since the last add
                trajectoryPath->m_timePassed += timePassedInSeconds;

                const uint32 particleSampleRate = 30;
                if (trajectoryPath->m_timePassed >= (1.0f / particleSampleRate) && distanceTraveledEnough)
                {
                    // create the particle, fill its data and add it to the trajectory trace path
                    MCommon::RenderUtil::TrajectoryPathParticle trajectoryParticle;
                    trajectoryParticle.m_worldTm = worldTM;
                    trajectoryPath->m_traceParticles.emplace_back(trajectoryParticle);

                    // reset the time passed as we just added a new particle
                    trajectoryPath->m_timePassed = 0.0f;
                }
            }

            // make sure we don't have too many items in our array
            if (trajectoryPath->m_traceParticles.size() > 50)
            {
                trajectoryPath->m_traceParticles.erase(begin(trajectoryPath->m_traceParticles));
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

        const AZStd::unordered_set<size_t>& visibleJointIndices = GetManager()->GetVisibleJointIndices();
        const AZStd::unordered_set<size_t>& selectedJointIndices = GetManager()->GetSelectedJointIndices();

        // render the AABBs
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_AABB))
        {
            MCommon::RenderUtil::AABBRenderSettings settings;
            settings.m_nodeBasedColor          = renderOptions->GetNodeAABBColor();
            settings.m_staticBasedColor        = renderOptions->GetStaticAABBColor();
            settings.m_meshBasedColor          = renderOptions->GetMeshAABBColor();

            renderUtil->RenderAabbs(actorInstance, settings);
        }

        if (widget->GetRenderFlag(RenderViewWidget::RENDER_LINESKELETON))
        {
            const MCommon::Camera* camera = widget->GetRenderWidget()->GetCamera();
            const AZ::Vector3& cameraPos = camera->GetPosition();

            AZ::Aabb aabb;
            actorInstance->CalcNodeBasedAabb(&aabb);
            const AZ::Vector3 aabbMid = aabb.GetCenter();
            const float aabbRadius = AZ::Vector3(aabb.GetMax() - aabb.GetMin()).GetLength() * 0.5f;
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
            const float jointSphereRadius = aabbRadius * scaleMultiplier * baseRadius;

            renderUtil->RenderSimpleSkeleton(actorInstance, &visibleJointIndices, &selectedJointIndices,
                renderOptions->GetLineSkeletonColor(), renderOptions->GetSelectedObjectColor(), jointSphereRadius);
        }

        bool cullingEnabled = renderUtil->GetCullingEnabled();
        bool lightingEnabled = renderUtil->GetLightingEnabled();
        renderUtil->EnableCulling(false); // disable culling
        renderUtil->EnableLighting(false); // disable lighting
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_SKELETON))
        {
            renderUtil->RenderSkeleton(actorInstance, emstudioActor->m_boneList, &visibleJointIndices, &selectedJointIndices, renderOptions->GetSkeletonColor(), renderOptions->GetSelectedObjectColor());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODEORIENTATION))
        {
            renderUtil->RenderNodeOrientations(actorInstance, emstudioActor->m_boneList, &visibleJointIndices, &selectedJointIndices, emstudioActor->m_normalsScaleMultiplier * renderOptions->GetNodeOrientationScale(), renderOptions->GetScaleBonesOnLength());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_ACTORBINDPOSE))
        {
            renderUtil->RenderBindPose(actorInstance);
        }

        // render motion extraction debug info
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_MOTIONEXTRACTION))
        {
            // render an arrow for the trajectory
            renderUtil->RenderTrajectoryPath(FindTracePath(actorInstance), renderOptions->GetTrajectoryArrowInnerColor(), emstudioActor->m_characterHeight * 0.05f);
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

            const size_t geomLODLevel   = actorInstance->GetLODLevel();
            const size_t numEnabled     = actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numEnabled; ++i)
            {
                EMotionFX::Node*    node      = emstudioActor->m_actor->GetSkeleton()->GetNode(actorInstance->GetEnabledNode(i));
                const size_t        nodeIndex = node->GetNodeIndex();
                EMotionFX::Mesh*    mesh      = emstudioActor->m_actor->GetMesh(geomLODLevel, nodeIndex);

                renderUtil->ResetCurrentMesh();

                if (!mesh)
                {
                    continue;
                }

                const AZ::Transform worldTM = pose->GetMeshNodeWorldSpaceTransform(geomLODLevel, nodeIndex).ToAZTransform();

                if (!mesh->GetIsCollisionMesh())
                {
                    renderUtil->RenderNormals(mesh, worldTM, renderVertexNormals, renderFaceNormals, renderOptions->GetVertexNormalsScale() * emstudioActor->m_normalsScaleMultiplier, renderOptions->GetFaceNormalsScale() * emstudioActor->m_normalsScaleMultiplier, renderOptions->GetVertexNormalsColor(), renderOptions->GetFaceNormalsColor());
                    if (renderTangents)
                    {
                        renderUtil->RenderTangents(mesh, worldTM, renderOptions->GetTangentsScale() * emstudioActor->m_normalsScaleMultiplier, renderOptions->GetTangentsColor(), renderOptions->GetMirroredBitangentsColor(), renderOptions->GetBitangentsColor());
                    }
                    if (renderWireframe)
                    {
                        renderUtil->RenderWireframe(mesh, worldTM, renderOptions->GetWireframeColor(), false, emstudioActor->m_normalsScaleMultiplier);
                    }
                }
                else
                if (renderCollisionMeshes)
                {
                    renderUtil->RenderWireframe(mesh, worldTM, renderOptions->GetCollisionMeshColor(), false, emstudioActor->m_normalsScaleMultiplier);
                }
            }
        }

        // render the selection
        if (renderOptions->GetRenderSelectionBox() && EMotionFX::GetActorManager().GetNumActorInstances() != 1 && GetCurrentSelection()->CheckIfHasActorInstance(actorInstance))
        {
            AZ::Aabb aabb = actorInstance->GetAabb();
            aabb.Expand(AZ::Vector3(0.005f));
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
