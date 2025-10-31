/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Integration/AnimationBus.h>
#include <Integration/EMotionFXBus.h>
#include <Integration/Rendering/RenderBackendManager.h>

#include <CrySystemBus.h>

#if defined (EMOTIONFXANIMATION_EDITOR)
#   include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#   include <AzToolsFramework/API/ToolsApplicationAPI.h>
#   include <AzToolsFramework/API/EditorAnimationSystemRequestBus.h>
#   include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#   include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#   include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#endif // EMOTIONFXANIMATION_EDITOR

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }
}

namespace AzToolsFramework
{
    class PropertyHandlerBase;
}

namespace EMotionFX
{
    namespace Integration
    {
        class EMotionFXEventHandler;

        class SystemComponent
            : public AZ::Component
            , private SystemRequestBus::Handler
            , private AZ::TickBus::Handler
            , private CrySystemEventBus::Handler
            , private EMotionFXRequestBus::Handler
            , private IRaycastRequests
#if defined (EMOTIONFXANIMATION_EDITOR)
            , private AzToolsFramework::EditorEvents::Bus::Handler
            , private AzToolsFramework::EditorAnimationSystemRequestsBus::Handler
            , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
            , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
#endif // EMOTIONFXANIMATION_EDITOR

        {
        public:
            AZ_COMPONENT(SystemComponent, "{7AE4102B-387C-4157-B8C7-8D1EA3BCFD60}");

            SystemComponent();
            ~SystemComponent() override;

            static void ReflectEMotionFX(AZ::ReflectContext* context);
            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        private:
            //unique_ptr cannot be copied -> vector of unique_ptrs cannot be copied -> class cannot be copied
            SystemComponent(const SystemComponent&) = delete;

            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZ::TickBus::Handler
            void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
            int GetTickOrder() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // CrySystemEventBus
            void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
            void OnCrySystemShutdown(ISystem&) override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // EMotionFXRequestBus
            void RegisterAnimGraphObjectType(EMotionFX::AnimGraphObject* objectTemplate) override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // IRaycastRequests overrides ...
            void EnableRayRequests() override;
            void DisableRayRequests() override;
            IRaycastRequests::RaycastResult Raycast(AZ::EntityId entityId, const IRaycastRequests::RaycastRequest& rayRequest) override;
            ////////////////////////////////////////////////////////////////////////

            void RegisterAssetTypesAndHandlers();
            void SetMediaRoot(const char* alias);

#if defined (EMOTIONFXANIMATION_EDITOR)
            void NotifyRegisterViews() override;
            bool IsSystemActive(EditorAnimationSystemRequests::AnimationSystem systemType) override;

            //////////////////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
            AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
            void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
            void AddSourceFileCreators(const char* fullSourceFolderName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators) override;
            //////////////////////////////////////////////////////////////////////////////////////

            // AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler...
            void OnActionContextRegistrationHook() override;

            bool HandlesSource(AZStd::string_view fileName) const;

            AZStd::vector<AzToolsFramework::PropertyHandlerBase*> m_propertyHandlers;
#endif // EMOTIONFXANIMATION_EDITOR

            AZ::u32 m_numThreads;

        private:
            //! Synchronize the actor instance location with the entity or character controller.
            //! In case no character controller component is available, the entity will be moved
            //! to the actor instance position. The spatial difference between the entity and the
            //! actor instance will be calculated in case a character controller is present, and the
            //! velocity will be applied to it to move it towards the actor instance.
            void ApplyMotionExtraction(const ActorInstance* actorInstance, float timeDelta);

            AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler> > m_assetHandlers;
            AZStd::unique_ptr<EMotionFXEventHandler> m_eventHandler;
            AZStd::unique_ptr<RenderBackendManager> m_renderBackendManager;

#if defined(EMOTIONFXANIMATION_EDITOR)
            AZStd::unique_ptr<EMStudio::EMStudioManager> m_emstudioManager;
#endif // EMOTIONFXANIMATION_EDITOR
        };
    }
}
