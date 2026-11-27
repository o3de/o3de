/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/InGameUI/UiFrameworkBus.h>

#include <LmbrCentral/Rendering/TextureAsset.h>
#include <ILevelSystem.h>

#include <Shine/Bus/UiSystemBus.h>
#include <Shine/Bus/UiCanvasManagerBus.h>
#include <Shine/Bus/Tools/UiSystemToolsBus.h>
#include <Shine/UiComponentTypes.h>
#include "Shine.h"

#if !defined(SHINE_BUILDER) && !defined(SHINE_TESTS)
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#endif

namespace Shine
{
    class ShineSystemComponent
        : public AZ::Component
        , protected UiSystemBus::Handler
        , protected UiSystemToolsBus::Handler
        , protected UiFrameworkBus::Handler
        , protected CrySystemEventBus::Handler
        , public ILevelSystemListener
    {
    public:
        AZ_COMPONENT(ShineSystemComponent, ShineSystemComponentUuid);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        static void SetShineComponentDescriptors(const AZStd::list<AZ::ComponentDescriptor*>* descriptors);

        ShineSystemComponent();

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiSystemBus interface implementation
        void RegisterComponentTypeForMenuOrdering(const AZ::Uuid& typeUuid) override;
        const AZStd::vector<AZ::Uuid>* GetComponentTypesForMenuOrdering() override;
        const AZStd::list<AZ::ComponentDescriptor*>* GetShineComponentDescriptors() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiSystemToolsInterface interface implementation
        CanvasAssetHandle* LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc) override;
        void SaveCanvasToStream(CanvasAssetHandle* canvas, AZ::IO::FileIOStream& stream) override;
        AZStd::vector<AZ::Entity*>& GetChildEntities(CanvasAssetHandle* canvas) override;
        AZ::Entity* GetCanvasEntity(CanvasAssetHandle* canvas) override;
        void ReplaceChildEntities(CanvasAssetHandle* canvas, AZStd::vector<AZ::Entity*> newEntities) override;
        void ReplaceCanvasEntity(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZ::Entity* newCanvasEntity) override;
        void DestroyCanvas(CanvasAssetHandle* canvas) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiFrameworkInterface interface implementation
        bool HasUiElementComponent(AZ::Entity* entity) override;
        void AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities) override;
        void HandleEditorOnlyEntities(const EntityList& exportEntities, const EntityIdSet& editorOnlyEntityIds) override;
        ////////////////////////////////////////////////////////////////////////

        // CrySystemEventBus ///////////////////////////////////////////////////////
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;
        ////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ILevelSystemListener interface implementation
        void OnUnloadComplete(const char* levelName) override;

        void BroadcastCursorImagePathname();

#if !defined(SHINE_BUILDER) && !defined(SHINE_TESTS)
        // Load pass template mappings for this gem
        void LoadPassTemplateMappings();
#endif

    protected:  // data

        AZStd::unique_ptr<IShine> m_Shine;

        AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_cursorImagePathname;

        // The components types registers in order to control their order in the add component
        // menu and the properties pane - may go away soon
        AZStd::vector<AZ::Uuid> m_componentTypes;

        // We only store this in order to generate metrics on Shine specific components
        static const AZStd::list<AZ::ComponentDescriptor*>* m_componentDescriptors;

#if !defined(SHINE_BUILDER) && !defined(SHINE_TESTS)
        AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
#endif
    };
}
