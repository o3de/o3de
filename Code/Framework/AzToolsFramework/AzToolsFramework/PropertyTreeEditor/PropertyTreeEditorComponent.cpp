/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditorComponent.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void PropertyTreeEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PropertyTreeEditorComponent, AZ::Component>();
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PropertyTreeEditor>("PropertyTreeEditor")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Property")
                    ->Attribute(AZ::Script::Attributes::Module, "property")

                    // Property tree information API
                    ->Method("BuildPathsList", &PropertyTreeEditor::BuildPathsList, nullptr, "Get a complete list of all property paths in the tree.")
                        ->Attribute(AZ::Script::Attributes::Alias, "build_paths_list")
                    ->Method("BuildPathsListWithTypes", &PropertyTreeEditor::BuildPathsListWithTypes, nullptr, "Get a complete list of all property paths in the tree with (typename)s.")
                        ->Attribute(AZ::Script::Attributes::Alias, "build_paths_list_with_types")

                    // Attribute & visibility API
                    ->Method("SetVisibleEnforcement", &PropertyTreeEditor::SetVisibleEnforcement, nullptr, "Limits the properties using the visibility flags such as ShowChildrenOnly.") 
                        ->Attribute(AZ::Script::Attributes::Alias, "set_visible_enforcement")
                    ->Method("HasAttribute", &PropertyTreeEditor::HasAttribute, nullptr, "Detects if a property has an attribute.")
                        ->Attribute(AZ::Script::Attributes::Alias, "has_attribute")

                    // Property value API
                    ->Method("GetProperty", &PropertyTreeEditor::GetProperty, nullptr, "Gets a property value.")
                        ->Attribute(AZ::Script::Attributes::Alias, "get_value")
                    ->Method("SetProperty", &PropertyTreeEditor::SetProperty, nullptr, "Sets a property value.")
                        ->Attribute(AZ::Script::Attributes::Alias, "set_value")
                    ->Method("CompareProperty", &PropertyTreeEditor::CompareProperty, nullptr, "Compares a property value.")
                        ->Attribute(AZ::Script::Attributes::Alias, "compare_value")

                    // Container API
                    ->Method("IsContainer", &PropertyTreeEditor::IsContainer, nullptr, "True if property path points to a container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "is_container")
                    ->Method("GetContainerCount", &PropertyTreeEditor::GetContainerCount, nullptr, "Returns the size of the container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "get_container_count")
                    ->Method("ResetContainer", &PropertyTreeEditor::ResetContainer, nullptr, "Clears the items in a container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "reset_container")
                    ->Method("AddContainerItem", &PropertyTreeEditor::AddContainerItem, nullptr, "Add an item in a container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "add_container_item")
                    ->Method("AppendContainerItem", &PropertyTreeEditor::AppendContainerItem, nullptr, "Appends an item in an non-associative container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "append_container_item")
                    ->Method("RemoveContainerItem", &PropertyTreeEditor::RemoveContainerItem, nullptr, "Removes a single item from a container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "remove_container_item")
                    ->Method("UpdateContainerItem", &PropertyTreeEditor::UpdateContainerItem, nullptr, "Updates an existing the item's value in a container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "update_container_item")
                    ->Method("GetContainerItem", &PropertyTreeEditor::GetContainerItem, nullptr, "Retrieves an item value from a container.")
                        ->Attribute(AZ::Script::Attributes::Alias, "get_container_item")
                    ;
            }
        }
    }
}
