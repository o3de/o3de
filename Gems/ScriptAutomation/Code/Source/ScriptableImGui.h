/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <imgui/imgui.h>

#define SCRIPTABLE_IMGUI

#ifdef SCRIPTABLE_IMGUI
#define Scriptable_ImGui ScriptAutomation::ScriptableImGui
#endif

namespace ScriptAutomation
{
    class ScriptAutomationSystemComponent;
    class ScriptBindings;

    //! Wraps the ImGui API in a reflection system that automatically exposes ImGui data elements
    //! to the scripting system. It enhances standard ImGui functions to check for scripted
    //! actions that can perform the same actions as a user.
    class ScriptableImGui final
    {
        friend class ScriptAutomationSystemComponent;
    public:
        AZ_CLASS_ALLOCATOR(ScriptableImGui, AZ::SystemAllocator, 0);

        //! Utility for calling ScriptableImGui::PushNameContext and ScriptableImGui::PopNameContext
        class ScopedNameContext final
        {
        public:
            ScopedNameContext(const AZStd::string& nameContext) { ScriptableImGui::PushNameContext(nameContext); }
            ~ScopedNameContext() { ScriptableImGui::PopNameContext(); }
        };

        //! This can be used to add some context around the ImGui labels that are exposed to the script system.
        //! Each call to PushNameContext() will add a prefix the ImGui labels to form the script field IDs.
        //! For example, the following will result in a script field ID of "A/B/MyButton" instead of just "MyButton".
        //!     PushNameContext("A");
        //!     PushNameContext("B");
        //!     Button("MyButton",...);
        //!     PopNameContext();
        //!     PopNameContext();
        //! This is especially useful for disambiguating similar ImGui labels.
        //! There is also a ScopedNameContext utility class for managing the push/pop using the call stack.
        static void PushNameContext(const AZStd::string& nameContext);
        static void PopNameContext();

        //////////////////////////////////////////////////////////////////
        // ImGui bridge functions...
        // These follow the same API as the corresponding ImGui functions.
        // Add more bridge functions as needed.

        static bool Begin(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);
        static void End();

        static bool Checkbox(const char* label, bool* v);
        static bool Button(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
        static bool ListBox(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items = -1);
        static bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1);

        static bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d");
        static bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
        static bool SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
        static bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
        static bool SliderAngle(const char* label, float* v, float v_min = -360.0f, float v_max = 360.0f, const char* format = "%.0f deg");

        static bool ColorEdit3(const char* label, float v[3], ImGuiColorEditFlags flags);
        static bool ColorPicker3(const char* label, float v[3], ImGuiColorEditFlags flags);

        static bool Selectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
        static bool Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));

        static bool TreeNodeEx(const char* label, ImGuiSelectableFlags flags = 0);
        static void TreePop();

        static bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
        static void EndCombo();

        static bool BeginMenu(const char* label, bool enabled = true);
        static void EndMenu();
        static bool MenuItem(const char* label, const char* shortcut = NULL, bool selected = false, bool enabled = true);

        static bool RadioButton(const char* label, int* v, int v_button);

        //! These functions are called through scripts to schedule a scripted action.
        //! This data will be consumed by subsequent calls to ImGui API functions.
        //! @param pathToImGuiItem - full path to an ImGui item, including the name context
        static void SetBool(const AZStd::string& name, bool value);
        static void SetNumber(const AZStd::string& name, float value);
        static void SetVector(const AZStd::string& name, const AZ::Vector2& value);
        static void SetVector(const AZStd::string& name, const AZ::Vector3& value);
        static void SetString(const AZStd::string& name, const AZStd::string& value);
    private:

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // Private API for ScriptManager to call...

        static void Create();
        static void Destory();

        //! Call this every frame to report errors when scripted actions aren't consumed through ImGui API function calls.
        //! This usually indicates that a script is trying to manipulate ImGui elements that don't exist.
        static void CheckAllActionsConsumed();

        //! Clears any scripted actions that were scheduled. This should be called every frame to make sure old actions
        //! don't hang around indefinitely and get consumed later and cause unexpected behavior.
        static void ClearActions();

        /////////////////////////////////////////////////////////////////////////////////////////////////

        using InvalidActionItem = AZStd::monostate;
        // Note we don't include an int type because Lua only supports floats
        using ActionItem = AZStd::variant<InvalidActionItem, bool, float, AZ::Vector2, AZ::Vector3, AZStd::string>;

        //! This utility function factors out common steps that most of the ImGui API bridge functions must perform.
        template<typename ActionDataT>
        static bool ActionHelper(
            const char* label,
            AZStd::function<bool()> imguiAction,
            AZStd::function<void(const AZStd::string& /*pathToImGuiItem*/)> reportScriptableAction,
            AZStd::function<bool(ActionDataT /*scriptArg*/)> handleScriptedAction,
            bool shouldReportScriptableActionAfterAnyChange = false);

        template<typename ImGuiActionType>
        static bool ThreeComponentHelper(const char* label, float v[3], ImGuiActionType& imguiAction);

        //! Finds a scheduled script action and removes it from the list of actions.
        //! @param pathToImGuiItem - full path to an ImGui item, including the name context
        static ActionItem FindAndRemoveAction(const AZStd::string& pathToImGuiItem);

        //! Makes a full script field ID path for the given ImGui label, by prefixing the current name context
        static AZStd::string MakeFullPath(const AZStd::string& forLabel);

        //! Utility function to ensure all script errors use a similar format
        static void ReportScriptError(const char* message);

        //! Provides a name context prefix to script field IDs for disambiguation.
        AZStd::vector<AZStd::string> m_nameContextStack; 

        using ActionMap = AZStd::unordered_map<AZStd::string, ActionItem>;
        ActionMap m_scriptedActions;

        bool m_isInScriptedComboPopup = false;

        static ScriptableImGui* s_instance;
    };

} // namespace ScriptAutomation
