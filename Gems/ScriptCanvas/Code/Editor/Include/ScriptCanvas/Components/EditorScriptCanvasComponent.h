/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponentSerializer.h>
#include <Editor/Framework/Configuration.h>

namespace ScriptCanvasEditor
{
    /*! EditorScriptCanvasComponent
    The user facing Editor Component for interacting with ScriptCanvas.
    Per graph instance variables values are stored here and injected into the runtime ScriptCanvas component in BuildGameEntity.
    */
    class EditorScriptCanvasComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorScriptCanvasComponent, "{C28E2D29-0746-451D-A639-7F113ECF5D72}", AzToolsFramework::Components::EditorComponentBase);

        friend class AZ::EditorScriptCanvasComponentSerializer;

        EditorScriptCanvasComponent();
        EditorScriptCanvasComponent(const SourceHandle& sourceHandle);
        ~EditorScriptCanvasComponent() override;
                        
    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }

        static void Reflect(AZ::ReflectContext* context);

        //=====================================================================
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        //=====================================================================
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        void SetPrimaryAsset(const AZ::Data::AssetId&) override;
        
        void MergeWithLatestCompilation(const ScriptCanvasBuilder::BuildVariableOverrides& buildData);

    private:
       Configuration m_configuration;
    };
}
