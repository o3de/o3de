/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/Spawnable/EditorInfoRemover.h>
#include <Prefab/Spawnable/PrefabProcessorContext.h>
#include <Prefab/PrefabSystemComponent.h>
#include <Prefab/PrefabDomTypes.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;

    class TestExportRuntimeComponentWithCallback
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestExportRuntimeComponentWithCallback, "{BD30EBBB-74DA-473C-9C68-7077AAE8C0B1}", AZ::Component);

        TestExportRuntimeComponentWithCallback() = default;
        TestExportRuntimeComponentWithCallback(bool returnPointerToSelf, bool exportHandled);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* context);

        AZ::ExportedComponent ExportComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/);

        const bool m_exportHandled{ false };
        const bool m_returnPointerToSelf{ false };
    };

    class TestExportRuntimeComponentWithoutCallback
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestExportRuntimeComponentWithoutCallback, "{44216269-2BAB-48E4-864F-F8D4CCFF60BB}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* context);
    };

    class TestExportEditorComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_COMPONENT(TestExportEditorComponent, "{60EE7F0E-1C89-433A-AA7C-20F64BA1F470}", AzToolsFramework::Components::EditorComponentBase);

        enum class ExportComponentType
        {
            ExportEditorComponent,
            ExportRuntimeComponentWithCallBack,
            ExportRuntimeComponentWithoutCallBack,
            ExportNullComponent,
        };

        TestExportEditorComponent() = default;
        TestExportEditorComponent(ExportComponentType exportType, bool exportHandled);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* context);

        AZ::ExportedComponent ExportComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/);
        
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        ExportComponentType m_exportType{ ExportComponentType::ExportNullComponent };
        bool m_exportHandled{ false };
    };

    class SpawnableRemoveEditorInfoTestFixture
        : public ToolsApplicationFixture<>
    {
    protected:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        // create entity containing the EditorOnly component to be processed
        void CreateSourceEntity(const char* name, bool editorOnly);

        // create entity containing the non editor-only component to be processed
        void CreateSourceTestExportRuntimeEntity(const char* name, bool returnPointerToSelf, bool exportHandled);

        // create entity containing the editor-only component to be processed
        void CreateSourceTestExportEditorEntity(
            const char* name,
            TestExportEditorComponent::ExportComponentType exportType,
            bool exportHandled);
        
        // Locate and return an entity from the exported entities
        AZ::Entity* GetRuntimeEntity(const char* entityName);

        void ConvertSourceEntitiesToPrefab();

        void ConvertRuntimePrefab(bool expectedResult = true);

        AZStd::vector<AZ::Entity*> m_sourceEntities;
        AZStd::vector<AZ::Entity*> m_runtimeEntities;
        AZ::SerializeContext* m_serializeContext{ nullptr };
        PrefabSystemComponent* m_prefabSystemComponent{ nullptr };
        PrefabConversionUtils::EditorInfoRemover m_editorInfoRemover;
        PrefabConversionUtils::PrefabProcessorContext m_prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        PrefabDom m_prefabDom;
    };
}
