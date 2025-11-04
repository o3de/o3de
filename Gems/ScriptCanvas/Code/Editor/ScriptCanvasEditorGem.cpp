/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#if defined (SCRIPTCANVAS_EDITOR)

#include <ScriptCanvasGem.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/SimpleAsset.h>

#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Data/DataRegistry.h>

#include <Editor/ReflectComponent.h>
#include <Editor/SystemComponent.h>

#include <Editor/Components/IconComponent.h>
#include <Editor/Model/EntityMimeDataHandler.h>

#include <Libraries/Libraries.h>

#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>

#include <Asset/EditorAssetSystemComponent.h>
#include <Builder/ScriptCanvasBuilderComponent.h>
#include <Editor/GraphCanvas/Components/DynamicOrderingDynamicSlotComponent.h>
#include <Editor/GraphCanvas/Components/DynamicSlotComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/AzEventHandlerNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ClassMethodNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventSenderNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusSenderNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/GetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/NodelingDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/SetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/UserDefinedNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/VariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/FunctionNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/FunctionDefinitionNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/MappingComponent.h>

#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

namespace ScriptCanvas
{
    ////////////////////////////////////////////////////////////////////////////
    // ScriptCanvasModule
    ////////////////////////////////////////////////////////////////////////////

    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    ScriptCanvasModule::ScriptCanvasModule()
        : ScriptCanvasModuleCommon()
    {
        m_descriptors.insert(m_descriptors.end(), {
            ScriptCanvasBuilder::PluginComponent::CreateDescriptor(),
            ScriptCanvasEditor::EditorAssetSystemComponent::CreateDescriptor(),
            ScriptCanvasEditor::EditorScriptCanvasComponent::CreateDescriptor(),
            ScriptCanvasEditor::EntityMimeDataHandler::CreateDescriptor(),
            ScriptCanvasEditor::EditorGraph::CreateDescriptor(),
            ScriptCanvasEditor::IconComponent::CreateDescriptor(),
            ScriptCanvasEditor::ReflectComponent::CreateDescriptor(),
            ScriptCanvasEditor::SystemComponent::CreateDescriptor(),
            ScriptCanvasEditor::EditorGraphVariableManagerComponent::CreateDescriptor(),
            ScriptCanvasEditor::VariablePropertiesComponent::CreateDescriptor(),
            ScriptCanvasEditor::SlotMappingComponent::CreateDescriptor(),
            ScriptCanvasEditor::SceneMemberMappingComponent::CreateDescriptor(),

            // GraphCanvas additions
            ScriptCanvasEditor::DynamicSlotComponent::CreateDescriptor(),
            ScriptCanvasEditor::DynamicOrderingDynamicSlotComponent::CreateDescriptor(),

            // Base Descriptor
            ScriptCanvasEditor::NodeDescriptorComponent::CreateDescriptor(),

            // Node Type Descriptor
            ScriptCanvasEditor::AzEventHandlerNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::ClassMethodNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::EBusHandlerNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::EBusHandlerEventNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::ScriptEventReceiverEventNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::ScriptEventReceiverNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::ScriptEventSenderNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::EBusSenderNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::VariableNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::GetVariableNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::SetVariableNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::UserDefinedNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::FunctionNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::FunctionDefinitionNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::NodelingDescriptorComponent::CreateDescriptor()
            });
    }

    AZ::ComponentTypeList ScriptCanvasModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = GetCommonSystemComponents();

        components.insert(components.end(), std::initializer_list<AZ::Uuid> {
                azrtti_typeid<ScriptCanvasEditor::EditorAssetSystemComponent>(),
                azrtti_typeid<ScriptCanvasEditor::ReflectComponent>(),
                azrtti_typeid<ScriptCanvasEditor::SystemComponent>()
        });

        return components;
    }
}

#include <QDir>

// Qt resources are defined in the ScriptCanvas static library, so we must
// initialize them manually
extern int qInitResources_ScriptCanvasEditorResources();
extern int qCleanupResources_ScriptCanvasEditorResources();
namespace {
    struct initializer {
        initializer() { qInitResources_ScriptCanvasEditorResources(); }
        ~initializer() { qCleanupResources_ScriptCanvasEditorResources(); }
    } dummy;
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), ScriptCanvas::ScriptCanvasModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ScriptCanvas_Editor, ScriptCanvas::ScriptCanvasModule)
#endif

#endif // SCRIPTCANVAS_EDITOR
