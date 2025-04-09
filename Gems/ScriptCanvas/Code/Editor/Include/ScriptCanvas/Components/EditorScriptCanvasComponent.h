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
#include <Editor/Framework/Configuration.h>
#include <ScriptCanvas/Components/EditorDeprecationData.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponentSerializer.h>

namespace ScriptCanvasEditor
{
    /*! EditorScriptCanvasComponent
    The user facing Editor Component for interacting with ScriptCanvas.
    Per graph instance variables values are stored here and injected into the runtime ScriptCanvas component in BuildGameEntity.
    */
    class EditorScriptCanvasComponent final
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorScriptCanvasComponent, "{C28E2D29-0746-451D-A639-7F113ECF5D72}", AzToolsFramework::Components::EditorComponentBase);

        enum Version
        {
            PrefabIntegration = 10,
            InternalDev,
            AddSourceHandle,
            RefactorAssets,
            RemoveRuntimeData,
            SeparateFromConfiguration,
            RefactorRuntime,

            // add description above
            Current
        };

        friend class AZ::EditorScriptCanvasComponentSerializer;

        EditorScriptCanvasComponent();
        EditorScriptCanvasComponent(const SourceHandle& sourceHandle);
        ~EditorScriptCanvasComponent() override;

        //=====================================================================
        // EditorComponentBase
        void SetPrimaryAsset(const AZ::Data::AssetId&) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ScriptCanvasService"));
        }

        static void Reflect(AZ::ReflectContext* context);

        //=====================================================================
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        //=====================================================================
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        Configuration m_configuration;
        AZ::EventHandler<const Configuration&> m_handlerSourceCompiled;
    };
}
