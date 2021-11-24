/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/SingleThreadScheduler.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/Source/EventDataFootIK.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/AnimGraphSyncTrack.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/ActorManager.h>

#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <MCore/Source/Command.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>

#include <EMotionFX/Source/PoseData.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>

#include <Integration/AnimationBus.h>
#include <Integration/EMotionFXBus.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Assets/MotionAsset.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/Assets/AnimGraphAsset.h>

#include <Integration/System/SystemComponent.h>
#include <Integration/System/CVars.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

#include <Integration/MotionExtractionBus.h>


#if defined(EMOTIONFXANIMATION_EDITOR) // EMFX tools / editor includes
// Qt
#include <QtGui/QSurfaceFormat>
// EMStudio tools and main window registration
#include <LyViewPaneNames.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzCore/std/string/wildcard.h>
#include <QApplication>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginManager.h>
// EMStudio plugins
#include <EMotionStudio/Plugins/StandardPlugins/Source/LogWindow/LogWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/CommandBar/CommandBarPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/ActionHistory/ActionHistoryPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/MorphTargetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/Attachments/AttachmentsPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/SceneManagerPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventsPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeGroups/NodeGroupsPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/RenderPlugins/Source/OpenGLRender/OpenGLRenderPlugin.h>
#include <Editor/Plugins/HitDetection/HitDetectionJointInspectorPlugin.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeInspectorPlugin.h>
#include <Editor/Plugins/Cloth/ClothJointInspectorPlugin.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Source/Editor/PropertyWidgets/PropertyTypes.h>
#include <EMotionFX_Traits_Platform.h>

#include <IEditor.h>
#endif // EMOTIONFXANIMATION_EDITOR

#include <IConsole.h>
#include <ISystem.h>

// include required AzCore headers
#include <AzCore/IO/FileIO.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        class EMotionFXEventHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(EMotionFXEventHandler, EMotionFXAllocator, 0);

            const AZStd::vector<EventTypes> GetHandledEventTypes() const override
            {
                return {
                           EVENT_TYPE_ON_EVENT,
                           EVENT_TYPE_ON_HAS_LOOPED,
                           EVENT_TYPE_ON_STATE_ENTERING,
                           EVENT_TYPE_ON_STATE_ENTER,
                           EVENT_TYPE_ON_STATE_END,
                           EVENT_TYPE_ON_STATE_EXIT,
                           EVENT_TYPE_ON_START_TRANSITION,
                           EVENT_TYPE_ON_END_TRANSITION
                };
            }

            /// Dispatch motion events to listeners via ActorNotificationBus::OnMotionEvent.
            void OnEvent(const EMotionFX::EventInfo& emfxInfo) override
            {
                const ActorInstance* actorInstance = emfxInfo.m_actorInstance;
                if (actorInstance)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();

                    // Fill engine-compatible structure to dispatch to game code.
                    MotionEvent motionEvent;
                    motionEvent.m_entityId = owningEntityId;
                    motionEvent.m_actorInstance = emfxInfo.m_actorInstance;
                    motionEvent.m_motionInstance = emfxInfo.m_motionInstance;
                    motionEvent.m_time = emfxInfo.m_timeValue;
                    // TODO
                    for (const auto& eventData : emfxInfo.m_event->GetEventDatas())
                    {
                        if (const EMotionFX::TwoStringEventData* twoStringEventData = azrtti_cast<const EMotionFX::TwoStringEventData*>(eventData.get()))
                        {
                            motionEvent.m_eventTypeName = twoStringEventData->GetSubject().c_str();
                            motionEvent.SetParameterString(twoStringEventData->GetParameters().c_str(), twoStringEventData->GetParameters().size());
                            break;
                        }
                    }
                    motionEvent.m_globalWeight = emfxInfo.m_globalWeight;
                    motionEvent.m_localWeight = emfxInfo.m_localWeight;
                    motionEvent.m_isEventStart = emfxInfo.IsEventStart();

                    // Queue the event to flush on the main thread.
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnMotionEvent, AZStd::move(motionEvent));
                }
            }

            void OnHasLooped(EMotionFX::MotionInstance* motionInstance) override
            {
                const ActorInstance* actorInstance = motionInstance->GetActorInstance();
                if (actorInstance)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnMotionLoop, motionInstance->GetMotion()->GetName());
                }
            }

            void OnStateEntering(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state) override
            {
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                if (actorInstance && state)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateEntering, state->GetName());
                }
            }

            void OnStateEnter(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state) override
            {
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                if (actorInstance && state)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateEntered, state->GetName());
                }
            }

            void OnStateEnd(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state) override
            {
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                if (actorInstance && state)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateExiting, state->GetName());
                }
            }

            void OnStateExit(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state) override
            {
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                if (actorInstance && state)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateExited, state->GetName());
                }
            }

            void OnStartTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition) override
            {
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                if (actorInstance)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();
                    const char* sourceName = transition->GetSourceNode() ? transition->GetSourceNode()->GetName() : "";
                    const char* targetName = transition->GetTargetNode() ? transition->GetTargetNode()->GetName() : "";
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateTransitionStart, sourceName, targetName);
                }
            }

            void OnEndTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition) override
            {
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                if (actorInstance)
                {
                    const AZ::EntityId owningEntityId = actorInstance->GetEntityId();
                    const char* sourceName = transition->GetSourceNode() ? transition->GetSourceNode()->GetName() : "";
                    const char* targetName = transition->GetTargetNode() ? transition->GetTargetNode()->GetName() : "";
                    ActorNotificationBus::QueueEvent(owningEntityId, &ActorNotificationBus::Events::OnStateTransitionEnd, sourceName, targetName);
                }
            }
        };

        //////////////////////////////////////////////////////////////////////////
        class ActorNotificationBusHandler
            : public ActorNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(ActorNotificationBusHandler, "{D2CD62E7-5FCF-4DC2-85DF-C205D5AB1E8B}", AZ::SystemAllocator,
                OnMotionEvent,
                OnMotionLoop,
                OnStateEntering,
                OnStateEntered,
                OnStateExiting,
                OnStateExited,
                OnStateTransitionStart,
                OnStateTransitionEnd);

            void OnMotionEvent(MotionEvent motionEvent) override
            {
                Call(FN_OnMotionEvent, motionEvent);
            }

            void OnMotionLoop(const char* motionName) override
            {
                Call(FN_OnMotionLoop, motionName);
            }

            void OnStateEntering(const char* stateName) override
            {
                Call(FN_OnStateEntering, stateName);
            }

            void OnStateEntered(const char* stateName) override
            {
                Call(FN_OnStateEntered, stateName);
            }

            void OnStateExiting(const char* stateName) override
            {
                Call(FN_OnStateExiting, stateName);
            }

            void OnStateExited(const char* stateName) override
            {
                Call(FN_OnStateExited, stateName);
            }

            void OnStateTransitionStart(const char* fromState, const char* toState) override
            {
                Call(FN_OnStateTransitionStart, fromState, toState);
            }

            void OnStateTransitionEnd(const char* fromState, const char* toState) override
            {
                Call(FN_OnStateTransitionEnd, fromState, toState);
            }
        };

        SystemComponent::~SystemComponent() = default;

        void SystemComponent::ReflectEMotionFX(AZ::ReflectContext* context)
        {
            MCore::ReflectionSerializer::Reflect(context);
            MCore::StringIdPoolIndex::Reflect(context);
            EMotionFX::ConstraintTransformRotationAngles::Reflect(context);

            // Actor
            EMotionFX::PhysicsSetup::Reflect(context);
            EMotionFX::SimulatedObjectSetup::Reflect(context);

            EMotionFX::PoseData::Reflect(context);
            EMotionFX::PoseDataRagdoll::Reflect(context);

            // Motion set
            EMotionFX::MotionSet::Reflect(context);
            EMotionFX::MotionSet::MotionEntry::Reflect(context);

            // Base AnimGraph objects
            EMotionFX::AnimGraphObject::Reflect(context);
            EMotionFX::AnimGraph::Reflect(context);
            EMotionFX::AnimGraphNodeGroup::Reflect(context);
            EMotionFX::AnimGraphGameControllerSettings::Reflect(context);

            // Anim graph objects
            EMotionFX::AnimGraphObjectFactory::ReflectTypes(context);

            // Anim graph's parameters
            EMotionFX::ParameterFactory::ReflectParameterTypes(context);

            EMotionFX::MotionEventTable::Reflect(context);
            EMotionFX::MotionEventTrack::Reflect(context);
            EMotionFX::AnimGraphSyncTrack::Reflect(context);
            EMotionFX::Event::Reflect(context);
            EMotionFX::MotionEvent::Reflect(context);
            EMotionFX::EventData::Reflect(context);
            EMotionFX::EventDataSyncable::Reflect(context);
            EMotionFX::TwoStringEventData::Reflect(context);
            EMotionFX::EventDataFootIK::Reflect(context);

            EMotionFX::Recorder::Reflect(context);

            EMotionFX::KeyTrackLinearDynamic<AZ::Vector3>::Reflect(context);
            EMotionFX::KeyTrackLinearDynamic<AZ::Quaternion>::Reflect(context);
            EMotionFX::KeyFrame<AZ::Vector3>::Reflect(context);
            EMotionFX::KeyFrame<AZ::Quaternion>::Reflect(context);

            MCore::Command::Reflect(context);
            CommandSystem::MotionIdCommandMixin::Reflect(context);
            CommandSystem::CommandAdjustMotion::Reflect(context);
            CommandSystem::CommandClearMotionEvents::Reflect(context);
            CommandSystem::CommandCreateMotionEventTrack::Reflect(context);
            CommandSystem::CommandAdjustMotionEventTrack::Reflect(context);
            CommandSystem::CommandCreateMotionEvent::Reflect(context);
            CommandSystem::CommandAdjustMotionEvent::Reflect(context);

            EMotionFX::CommandAdjustSimulatedObject::Reflect(context);
            EMotionFX::CommandAdjustSimulatedJoint::Reflect(context);

            EMotionFX::CommandAddRagdollJoint::Reflect(context);
            EMotionFX::CommandAdjustRagdollJoint::Reflect(context);
            EMotionFX::CommandRemoveRagdollJoint::Reflect(context);
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Reflect(AZ::ReflectContext* context)
        {
            ReflectEMotionFX(context);

            // Reflect component for serialization.
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SystemComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("NumThreads", &SystemComponent::m_numThreads)
                ;

                serializeContext->Class<MotionEvent>()
                    ->Version(1)
                ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<SystemComponent>("EMotion FX Animation", "Enables the EMotion FX animation solution")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_numThreads, "Number of threads", "Number of threads used internally by EMotion FX")
                    ;
                }
            }

            // Reflect system-level types and EBuses to behavior context.
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<SystemRequestBus>("SystemRequestBus")
                ;

                behaviorContext->EBus<SystemNotificationBus>("SystemNotificationBus")
                ;

                // In order for a property to be displayed in ScriptCanvas. Both a setter and a getter are necessary(both must be non-null).
                // This is being worked on in dragon branch, once this is complete the dummy lambda functions can be removed.
                behaviorContext->Class<MotionEvent>("MotionEvent")
                    ->Property("entityId", BehaviorValueGetter(&MotionEvent::m_entityId), [](MotionEvent*, const AZ::EntityId&) {})
                    ->Property("parameter", BehaviorValueGetter(&MotionEvent::m_parameter), [](MotionEvent*, const char*) {})
                    ->Property("eventType", BehaviorValueGetter(&MotionEvent::m_eventType), [](MotionEvent*, const AZ::u32&) {})
                    ->Property("eventTypeName", BehaviorValueGetter(&MotionEvent::m_eventTypeName), [](MotionEvent*, const char*) {})
                    ->Property("time", BehaviorValueGetter(&MotionEvent::m_time), [](MotionEvent*, const float&) {})
                    ->Property("globalWeight", BehaviorValueGetter(&MotionEvent::m_globalWeight), [](MotionEvent*, const float&) {})
                    ->Property("localWeight", BehaviorValueGetter(&MotionEvent::m_localWeight), [](MotionEvent*, const float&) {})
                    ->Property("isEventStart", BehaviorValueGetter(&MotionEvent::m_isEventStart), [](MotionEvent*, const bool&) {})
                ;

                behaviorContext->EBus<ActorNotificationBus>("ActorNotificationBus")
                    ->Handler<ActorNotificationBusHandler>()
                    ->Event("OnMotionEvent", &ActorNotificationBus::Events::OnMotionEvent)
                    ->Event("OnMotionLoop", &ActorNotificationBus::Events::OnMotionLoop)
                    ->Event("OnStateEntering", &ActorNotificationBus::Events::OnStateEntering)
                    ->Event("OnStateEntered", &ActorNotificationBus::Events::OnStateEntered)
                    ->Event("OnStateExiting", &ActorNotificationBus::Events::OnStateExiting)
                    ->Event("OnStateExited", &ActorNotificationBus::Events::OnStateExited)
                    ->Event("OnStateTransitionStart", &ActorNotificationBus::Events::OnStateTransitionStart)
                    ->Event("OnStateTransitionEnd", &ActorNotificationBus::Events::OnStateTransitionEnd)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
            dependent.push_back(AZ_CRC("JobsService", 0xd5ab5a50));
        }

        //////////////////////////////////////////////////////////////////////////
        SystemComponent::SystemComponent()
            : m_numThreads(1)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Init()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Activate()
        {
            // Start EMotionFX allocator.
            AZ::AllocatorInstance<EMotionFXAllocator>::Create();

            // Initialize MCore, which is EMotionFX's standard library of containers and systems.
            MCore::Initializer::InitSettings coreSettings;
            coreSettings.m_memAllocFunction = &EMotionFXAlloc;
            coreSettings.m_memReallocFunction = &EMotionFXRealloc;
            coreSettings.m_memFreeFunction = &EMotionFXFree;
            if (!MCore::Initializer::Init(&coreSettings))
            {
                AZ_Error("EMotion FX Animation", false, "Failed to initialize EMotion FX SDK Core");
                return;
            }

            // Initialize EMotionFX runtime.
            EMotionFX::Initializer::InitSettings emfxSettings;
            emfxSettings.m_unitType = MCore::Distance::UNITTYPE_METERS;

            if (!EMotionFX::Initializer::Init(&emfxSettings))
            {
                AZ_Error("EMotion FX Animation", false, "Failed to initialize EMotion FX SDK Runtime");
                return;
            }

            SetMediaRoot("@products@");
            // \todo Right now we're pointing at the @projectroot@ location (source) and working from there, because .actor and .motion (motion) aren't yet processed through
            // the scene pipeline. Once they are, we'll need to update various segments of the Tool to always read from the @products@ cache, but write to the @projectroot@ data/metadata.
            EMotionFX::GetEMotionFX().InitAssetFolderPaths();

            // Register EMotionFX event handler
            m_eventHandler.reset(aznew EMotionFXEventHandler());
            EMotionFX::GetEventManager().AddEventHandler(m_eventHandler.get());

            // Setup asset types.
            RegisterAssetTypesAndHandlers();

            SystemRequestBus::Handler::BusConnect();
            AZ::TickBus::Handler::BusConnect();
            CrySystemEventBus::Handler::BusConnect();
            EMotionFXRequestBus::Handler::BusConnect();
            EnableRayRequests();


            m_renderBackendManager = AZStd::make_unique<RenderBackendManager>();
#if defined (EMOTIONFXANIMATION_EDITOR)
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
            AzToolsFramework::EditorAnimationSystemRequestsBus::Handler::BusConnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();

            // Register custom property handlers for the reflected property editor.
            m_propertyHandlers = RegisterPropertyTypes();
#endif // EMOTIONFXANIMATION_EDITOR
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::Deactivate()
        {
#if defined(EMOTIONFXANIMATION_EDITOR)
            // Unregister custom property handlers for the reflected property editor.
            UnregisterPropertyTypes(m_propertyHandlers);
            m_propertyHandlers.clear();

            if (EMStudio::GetManager())
            {
                m_emstudioManager.reset();
                MysticQt::Initializer::Shutdown();
            }

            {
                using namespace AzToolsFramework;
                EditorRequests::Bus::Broadcast(&EditorRequests::UnregisterViewPane, EMStudio::MainWindow::GetEMotionFXPaneName());
            }

            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorAnimationSystemRequestsBus::Handler::BusDisconnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
#endif // EMOTIONFXANIMATION_EDITOR

            m_renderBackendManager.reset();

            EMotionFX::GetEventManager().RemoveEventHandler(m_eventHandler.get());
            m_eventHandler.reset();

            AZ::TickBus::Handler::BusDisconnect();
            CrySystemEventBus::Handler::BusDisconnect();
            EMotionFXRequestBus::Handler::BusDisconnect();
            DisableRayRequests();

            if (SystemRequestBus::Handler::BusIsConnected())
            {
                SystemRequestBus::Handler::BusDisconnect();

                m_assetHandlers.resize(0);

                EMotionFX::Initializer::Shutdown();
                MCore::Initializer::Shutdown();
            }

            // Memory leaks will be reported.
            AZ::AllocatorInstance<EMotionFXAllocator>::Destroy();
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::EnableRayRequests()
        {
            RaycastRequestBus::Handler::BusDisconnect();
            RaycastRequestBus::Handler::BusConnect();
        }

        void SystemComponent::DisableRayRequests()
        {
            RaycastRequestBus::Handler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::OnCrySystemInitialized([[maybe_unused]] ISystem& system, const SSystemInitParams&)
        {
#if !defined(AZ_MONOLITHIC_BUILD)
            // When module is linked dynamically, we must set our gEnv pointer.
            // When module is linked statically, we'll share the application's gEnv pointer.
            gEnv = system.GetGlobalEnvironment();
#endif

            REGISTER_CVAR2("emfx_updateEnabled", &CVars::emfx_updateEnabled, 1, VF_DEV_ONLY, "Enable main EMFX update");
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::OnCrySystemShutdown(ISystem&)
        {
            gEnv->pConsole->UnregisterVariable("emfx_updateEnabled");

#if !defined(AZ_MONOLITHIC_BUILD)
            gEnv = nullptr;
#endif
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::OnTick(float delta, [[maybe_unused]]AZ::ScriptTimePoint timePoint)
        {
            // Flush events prior to updating EMotion FX.
            ActorNotificationBus::ExecuteQueuedEvents();

            if (CVars::emfx_updateEnabled)
            {
                // Main EMotionFX runtime update.
                GetEMotionFX().Update(delta);

                bool inGameMode = true;
#if defined (EMOTIONFXANIMATION_EDITOR)
                // Check if we are in game mode.
                IEditor* editor = nullptr;
                AzToolsFramework::EditorRequestBus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
                inGameMode = !editor || editor->IsInGameMode();
#endif

                // Apply the motion extraction deltas to the character controller / entity transform for all entities.
                const ActorManager* actorManager = GetEMotionFX().GetActorManager();
                const size_t numActorInstances = actorManager->GetNumActorInstances();
                for (size_t i = 0; i < numActorInstances; ++i)
                {
                    ActorInstance* actorInstance = actorManager->GetActorInstance(i);

                    // Apply motion extraction only in game mode or in case the actor instance belongs to the Animation Editor.
                    const bool applyMotionExtraction = inGameMode || !actorInstance->GetIsOwnedByRuntime();
                    if (applyMotionExtraction)
                    {
                        actorInstance->SetMotionExtractionEnabled(true);
                        ApplyMotionExtraction(actorInstance, delta);
                    }
                    else
                    {
                        actorInstance->SetMotionExtractionEnabled(false);
                    }
                }
            }
        }

        void SystemComponent::ApplyMotionExtraction(const ActorInstance* actorInstance, float timeDelta)
        {
            AZ_Assert(actorInstance, "Cannot apply motion extraction. Actor instance is not valid.");
            AZ_Assert(actorInstance->GetActor(), "Cannot apply motion extraction. Actor instance is not linked to a valid actor.");

            AZ::Entity* entity = actorInstance->GetEntity();
            const Actor* actor = actorInstance->GetActor();
            if (!actorInstance->GetIsEnabled() ||
                !entity ||
                !actor->GetMotionExtractionNode())
            {
                return;
            }

            const AZ::EntityId entityId = entity->GetId();

            // Check if we have any physics character controllers.
            bool hasCustomMotionExtractionController = false;
            bool hasPhysicsController = false;

            Physics::CharacterRequestBus::EventResult(hasPhysicsController, entityId, &Physics::CharacterRequests::IsPresent);
            if (!hasPhysicsController)
            {
                hasCustomMotionExtractionController = MotionExtractionRequestBus::FindFirstHandler(entityId) != nullptr;
            }

            // If we have a physics controller.
            if (hasCustomMotionExtractionController || hasPhysicsController)
            {
                const float deltaTimeInv = (timeDelta > 0.0f) ? (1.0f / timeDelta) : 0.0f;

                AZ::Transform currentTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(currentTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);

                const AZ::Vector3 actorInstancePosition = actorInstance->GetWorldSpaceTransform().m_position;
                const AZ::Vector3 positionDelta = actorInstancePosition - currentTransform.GetTranslation();

                if (hasPhysicsController)
                {
                    Physics::CharacterRequestBus::Event(
                        entityId, &Physics::CharacterRequests::AddVelocity, positionDelta * deltaTimeInv);
                }
                else if (hasCustomMotionExtractionController)
                {
                    MotionExtractionRequestBus::Event(entityId, &MotionExtractionRequestBus::Events::ExtractMotion, positionDelta, timeDelta);
                    AZ::TransformBus::EventResult(currentTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
                }

                // Update the entity rotation.
                const AZ::Quaternion actorInstanceRotation = actorInstance->GetWorldSpaceTransform().m_rotation;
                const AZ::Quaternion currentRotation = currentTransform.GetRotation();
                if (!currentRotation.IsClose(actorInstanceRotation, AZ::Constants::FloatEpsilon))
                {
                    AZ::Transform newTransform = currentTransform;
                    newTransform.SetRotation(actorInstanceRotation);
                    AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, newTransform);
                }
            }
            else // There is no physics controller, just use EMotion FX's actor instance transform directly.
            {
                const AZ::Transform newTransform = actorInstance->GetWorldSpaceTransform().ToAZTransform();
                AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, newTransform);
            }
        }

        int SystemComponent::GetTickOrder()
        {
            return AZ::TICK_ANIMATION;
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::RegisterAnimGraphObjectType(EMotionFX::AnimGraphObject* objectTemplate)
        {
            EMotionFX::AnimGraphObjectFactory::GetUITypes().emplace(azrtti_typeid(objectTemplate));
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::RegisterAssetTypesAndHandlers()
        {
            // Initialize asset handlers.
            m_assetHandlers.emplace_back(aznew ActorAssetHandler);
            m_assetHandlers.emplace_back(aznew MotionAssetHandler);
            m_assetHandlers.emplace_back(aznew MotionSetAssetHandler);
            m_assetHandlers.emplace_back(aznew AnimGraphAssetHandler);

            // Add asset types and extensions to AssetCatalog.
            auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
            if (assetCatalog)
            {
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<ActorAsset>());
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<MotionAsset>());
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<MotionSetAsset>());
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<AnimGraphAsset>());

                assetCatalog->AddExtension("actor");        // Actor
                assetCatalog->AddExtension("motion");       // Motion
                assetCatalog->AddExtension("motionset");    // Motion set
                assetCatalog->AddExtension("animgraph");    // Anim graph
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::SetMediaRoot(const char* alias)
        {
            const char* rootPath = AZ::IO::FileIOBase::GetInstance()->GetAlias(alias);
            if (rootPath)
            {
                AZStd::string mediaRootPath = rootPath;
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, mediaRootPath);
                EMotionFX::GetEMotionFX().SetMediaRootFolder(mediaRootPath.c_str());
            }
            else
            {
                AZ_Warning("EMotionFX", false, "Failed to set media root because alias \"%s\" could not be resolved.", alias);
            }
        }


        //////////////////////////////////////////////////////////////////////////
        RaycastRequests::RaycastResult SystemComponent::Raycast([[maybe_unused]] AZ::EntityId entityId, const RaycastRequests::RaycastRequest& rayRequest)
        {
            RaycastRequests::RaycastResult rayResult;

            // Build the ray request in the physics system.
            AzPhysics::RayCastRequest physicsRayRequest;
            physicsRayRequest.m_start       = rayRequest.m_start;
            physicsRayRequest.m_direction   = rayRequest.m_direction;
            physicsRayRequest.m_distance    = rayRequest.m_distance;
            physicsRayRequest.m_queryType   = rayRequest.m_queryType;

            // Cast the ray in the physics system.
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                    sceneHandle != AzPhysics::InvalidSceneHandle)
                {
                    AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(sceneHandle, &physicsRayRequest);
                    if (result) // We intersected.
                    {
                        rayResult.m_position = result.m_hits[0].m_position;
                        rayResult.m_normal = result.m_hits[0].m_normal;
                        rayResult.m_intersected = true;
                    }
                }
            }
            return rayResult;
        }


#if defined (EMOTIONFXANIMATION_EDITOR)

        //////////////////////////////////////////////////////////////////////////
        void InitializeEMStudioPlugins()
        {
            // Register EMFX plugins.
            EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();
            pluginManager->RegisterPlugin(new EMStudio::LogWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::CommandBarPlugin());
            pluginManager->RegisterPlugin(new EMStudio::ActionHistoryPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MotionWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MorphTargetsWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::TimeViewPlugin());
            pluginManager->RegisterPlugin(new EMStudio::AttachmentsPlugin());
            pluginManager->RegisterPlugin(new EMStudio::SceneManagerPlugin());
            pluginManager->RegisterPlugin(new EMStudio::NodeWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MotionEventsPlugin());
            pluginManager->RegisterPlugin(new EMStudio::MotionSetsWindowPlugin());
            pluginManager->RegisterPlugin(new EMStudio::NodeGroupsPlugin());
            pluginManager->RegisterPlugin(new EMStudio::AnimGraphPlugin());
            pluginManager->RegisterPlugin(new EMStudio::OpenGLRenderPlugin());
            pluginManager->RegisterPlugin(new EMotionFX::HitDetectionJointInspectorPlugin());
            pluginManager->RegisterPlugin(new EMotionFX::SkeletonOutlinerPlugin());
            pluginManager->RegisterPlugin(new EMotionFX::RagdollNodeInspectorPlugin());
            pluginManager->RegisterPlugin(new EMotionFX::ClothJointInspectorPlugin());
            pluginManager->RegisterPlugin(new EMotionFX::SimulatedObjectWidget());

            SystemNotificationBus::Broadcast(&SystemNotificationBus::Events::OnRegisterPlugin);
        }

        //////////////////////////////////////////////////////////////////////////
        void SystemComponent::NotifyRegisterViews()
        {
            using namespace AzToolsFramework;

            // Construct data folder that is used by the tool for loading assets (images etc.).
            auto editorAssetsPath = (AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / "Gems/EMotionFX/Assets/Editor").LexicallyNormal();

            // Re-initialize EMStudio.
            int argc = 0;
            char** argv = nullptr;

            MysticQt::Initializer::Init("", editorAssetsPath.c_str());
            m_emstudioManager = AZStd::make_unique<EMStudio::EMStudioManager>(qApp, argc, argv);

            InitializeEMStudioPlugins();

            // Get the MainWindow the first time so it is constructed
            EMStudio::GetManager()->GetMainWindow();

            EMStudio::GetManager()->ExecuteApp();

            AZStd::function<QWidget*(QWidget*)> windowCreationFunc = []([[maybe_unused]] QWidget* parent = nullptr)
                {
                    return EMStudio::GetMainWindow();
                };

            // Register EMotionFX window with the main editor.
            AzToolsFramework::ViewPaneOptions emotionFXWindowOptions;
            emotionFXWindowOptions.isPreview = false;
            emotionFXWindowOptions.isDeletable = true;
            emotionFXWindowOptions.isDockable = false;
#if AZ_TRAIT_EMOTIONFX_MAIN_WINDOW_DETACHED
            emotionFXWindowOptions.detachedWindow = true;
#endif
            emotionFXWindowOptions.optionalMenuText = "Animation Editor";
            emotionFXWindowOptions.showOnToolsToolbar = true;
            emotionFXWindowOptions.toolbarIcon = ":/Menu/emfx_editor.svg";

            EditorRequests::Bus::Broadcast(&EditorRequests::RegisterViewPane, EMStudio::MainWindow::GetEMotionFXPaneName(), LyViewPane::CategoryTools, emotionFXWindowOptions, windowCreationFunc);
        }

        //////////////////////////////////////////////////////////////////////////
        bool SystemComponent::IsSystemActive(EditorAnimationSystemRequests::AnimationSystem systemType)
        {
            return (systemType == AnimationSystem::EMotionFX);
        }

        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        AzToolsFramework::AssetBrowser::SourceFileDetails SystemComponent::GetSourceFileDetails(const char* fullSourceFileName)
        {
            using namespace AzToolsFramework::AssetBrowser;
            if (AZStd::wildcard_match("*.motionset", fullSourceFileName))
            {
                return SourceFileDetails("Editor/Images/AssetBrowser/MotionSet_16.svg");
            }
            else if (AZStd::wildcard_match("*.animgraph", fullSourceFileName))
            {
                return SourceFileDetails("Editor/Images/AssetBrowser/AnimGraph_16.svg");
            }
            return SourceFileDetails(); // no result
        }

#endif // EMOTIONFXANIMATION_EDITOR
    }
}
