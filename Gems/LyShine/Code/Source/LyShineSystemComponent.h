/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <AzFramework/InGameUI/UiFrameworkBus.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <ILevelSystem.h>

#include <LyShine/Bus/UiSystemBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/Tools/UiSystemToolsBus.h>
#include <LyShine/UiComponentTypes.h>
#include "LyShine.h"

#if !defined(LYSHINE_BUILDER) && !defined(LYSHINE_TESTS)
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#endif

namespace LyShine
{
    // LyShine depends on the LegacyAllocator. This will be managed
    // by the LyShineSystemComponent
    using LyShineAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator>;

    class LyShineSystemComponent
        : public AZ::Component
        , protected UiSystemBus::Handler
        , protected UiSystemToolsBus::Handler
        , protected LyShineAllocatorScope
        , protected UiFrameworkBus::Handler
        , protected CrySystemEventBus::Handler
        , public ILevelSystemListener
    {
    public:
        AZ_COMPONENT(LyShineSystemComponent, lyShineSystemComponentUuid);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        static void SetLyShineComponentDescriptors(const AZStd::list<AZ::ComponentDescriptor*>* descriptors);

        LyShineSystemComponent();

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
        const AZStd::list<AZ::ComponentDescriptor*>* GetLyShineComponentDescriptors() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiSystemToolsInterface interface implementation
        CanvasAssetHandle* LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc) override;
        void SaveCanvasToStream(CanvasAssetHandle* canvas, AZ::IO::FileIOStream& stream) override;
        AZ::SliceComponent* GetRootSliceSliceComponent(CanvasAssetHandle* canvas) override;
        AZ::Entity* GetRootSliceEntity(CanvasAssetHandle* canvas) override;
        AZ::Entity* GetCanvasEntity(CanvasAssetHandle* canvas) override;
        void ReplaceRootSliceSliceComponent(CanvasAssetHandle* canvas, AZ::SliceComponent* newSliceComponent) override;
        void ReplaceCanvasEntity(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZ::Entity* newCanvasEntity) override;
        void DestroyCanvas(CanvasAssetHandle* canvas) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiFrameworkInterface interface implementation
        bool HasUiElementComponent(AZ::Entity* entity) override;
        void AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities) override;
        void HandleEditorOnlyEntities(const EntityList& exportSliceEntities, const EntityIdSet& editorOnlyEntityIds) override;
        ////////////////////////////////////////////////////////////////////////

        // CrySystemEventBus ///////////////////////////////////////////////////////
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;
        ////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ILevelSystemListener interface implementation
        void OnUnloadComplete(const char* levelName) override;

        void BroadcastCursorImagePathname();

#if !defined(LYSHINE_BUILDER) && !defined(LYSHINE_TESTS)
        // Load pass template mappings for this gem
        void LoadPassTemplateMappings();
#endif

    protected:  // data

        CLyShine* m_pLyShine = nullptr;

        AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_cursorImagePathname;

        // The components types registers in order to control their order in the add component
        // menu and the properties pane - may go away soon
        AZStd::vector<AZ::Uuid> m_componentTypes;

        // We only store this in order to generate metrics on LyShine specific components
        static const AZStd::list<AZ::ComponentDescriptor*>* m_componentDescriptors;

#if !defined(LYSHINE_BUILDER) && !defined(LYSHINE_TESTS)
        AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
#endif
    };
}
