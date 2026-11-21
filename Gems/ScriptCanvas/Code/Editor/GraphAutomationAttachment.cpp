/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/GraphAutomation.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Components/EditorUtils.h>

namespace ScriptCanvasEditor
{
    namespace
    {
        //! The asset sub ID that names a source file itself rather than one of its products.
        constexpr AZ::u32 SourceAssetSubId = 0;
    } // namespace

    AZ::Outcome<EditorScriptCanvasComponent*, AZStd::string> GraphAutomation::ScriptComponent(const AZ::EntityComponentIdPair& pair)
    {
        const AZ::EntityId entityId = pair.GetEntityId();

        bool editorEntity = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntity, &AzToolsFramework::EditorEntityContextRequests::IsEditorEntity, entityId);

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

        EditorScriptCanvasComponent* component = nullptr;
        if (editorEntity && entity)
        {
            component = azrtti_cast<EditorScriptCanvasComponent*>(entity->FindComponent(pair.GetComponentId()));
        }

        if (!component)
        {
            return AZ::Failure(AZStd::string("Expected an attached, enabled ScriptCanvas editor component in this context"));
        }

        return AZ::Success(component);
    }

    AutomationResult GraphAutomation::Attach(const AZ::EntityComponentIdPair& pair, const AZStd::string& text)
    {
        auto window = Window();
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        auto component = ScriptComponent(pair);
        if (!component)
        {
            return AZ::Failure(component.GetError());
        }

        auto path = SourcePath(text);
        if (!path)
        {
            return AZ::Failure(path.GetError());
        }

        auto source = CreateFromAnyPath(SourceHandle(), path.GetValue());
        if (!source || !source->IsDescriptionValid())
        {
            return AZ::Failure(AZStd::string("Asset Processor has not registered this ScriptCanvas source"));
        }

        AzToolsFramework::ScopedUndoBatch undo("ScriptCanvas automation: attach source");

        // Select the source the same way the component's asset picker does, so the change is undoable and saved.
        component.GetValue()->SetPrimaryAsset(AZ::Data::AssetId(source->Id(), SourceAssetSubId));
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::Event(
            pair.GetEntityId(),
            &AzToolsFramework::PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged,
            pair.GetComponentId());
        undo.MarkEntityDirty(pair.GetEntityId());

        return Attachment(pair);
    }

    AutomationResult GraphAutomation::Attachment(const AZ::EntityComponentIdPair& pair)
    {
        auto window = Window();
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        auto component = ScriptComponent(pair);
        if (!component)
        {
            return AZ::Failure(component.GetError());
        }

        const SourceHandle& source = component.GetValue()->GetSource();
        return AZ::Success(AutomationValueMap{ { "selected", source.IsDescriptionValid() },
                                               { "sourceUuid", source.Id().ToString<AZStd::string>() },
                                               { "path", source.AbsolutePath().Native() } });
    }
} // namespace ScriptCanvasEditor
