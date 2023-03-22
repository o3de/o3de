/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>

#include <ScriptableImGui.h>
#include <Utils.h>

namespace ScriptAutomation
{
    ScriptableImGui* ScriptableImGui::s_instance = nullptr;

    void ScriptableImGui::Create()
    {
        AZ_Assert(s_instance == nullptr, "instance already called");
        s_instance = aznew ScriptableImGui();
    }

    void ScriptableImGui::Destory()
    {
        AZ_Assert(s_instance != nullptr, "instance is null");
        delete s_instance;
        s_instance = nullptr;
    }

    void ScriptableImGui::CheckAllActionsConsumed()
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);

        AZ_Error("Automation", s_instance->m_scriptedActions.empty(), "Not all scripted ImGui actions were consumed");
        for (auto iter : s_instance->m_scriptedActions)
        {
            AZ_Error("Automation", false, "Scripted action for '%s' not consumed", iter.first.c_str());
        }

        AZ_Error("Automation", s_instance->m_nameContextStack.empty(), "PushNameContext and PopNameContext calls didn't match");
    }

    void ScriptableImGui::ClearActions()
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);

        s_instance->m_scriptedActions.clear();
        s_instance->m_nameContextStack.clear();
    }

    void ScriptableImGui::PushNameContext(const AZStd::string& nameContext)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);

        s_instance->m_nameContextStack.push_back(nameContext);
    }

    void ScriptableImGui::PopNameContext()
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);

        AZ_Assert(!s_instance->m_nameContextStack.empty(), "Called PopNameContext too many times");
        s_instance->m_nameContextStack.pop_back();
    }

    ScriptableImGui::ActionItem ScriptableImGui::FindAndRemoveAction(const AZStd::string& pathToImGuiItem)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);

        auto iter = s_instance->m_scriptedActions.find(pathToImGuiItem);
        if (iter != s_instance->m_scriptedActions.end())
        {
            ScriptableImGui::ActionItem item = AZStd::move(iter->second);
            s_instance->m_scriptedActions.erase(iter);
            return AZStd::move(item);
        }

        return ScriptableImGui::ActionItem{};
    }

    AZStd::string ScriptableImGui::MakeFullPath(const AZStd::string& forLabel)
    {
        static constexpr char Delimiter[] = "/";

        AZStd::string fullPath;
        if (!s_instance->m_nameContextStack.empty())
        {
            AzFramework::StringFunc::Join(fullPath, s_instance->m_nameContextStack.begin(), s_instance->m_nameContextStack.end(), Delimiter);
            fullPath += Delimiter;
        }
        fullPath += forLabel;

        return fullPath;
    }

    void ScriptableImGui::ReportScriptError([[maybe_unused]] const char* message)
    {
        AZ_Error("Automation", false, "Script: %s", message);
    }

    void ScriptableImGui::SetBool(const AZStd::string& pathToImGuiItem, bool value)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);
        s_instance->m_scriptedActions[pathToImGuiItem] = value;
    }

    void ScriptableImGui::SetNumber(const AZStd::string& pathToImGuiItem, float value)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);
        s_instance->m_scriptedActions[pathToImGuiItem] = value;
    }

    void ScriptableImGui::SetVector(const AZStd::string& pathToImGuiItem, const AZ::Vector2& value)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);
        s_instance->m_scriptedActions[pathToImGuiItem] = value;
    }

    void ScriptableImGui::SetVector(const AZStd::string& pathToImGuiItem, const AZ::Vector3& value)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);
        s_instance->m_scriptedActions[pathToImGuiItem] = value;
    }

    void ScriptableImGui::SetString(const AZStd::string& pathToImGuiItem, const AZStd::string& value)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);
        s_instance->m_scriptedActions[pathToImGuiItem] = value;
    }

    template<typename ActionDataT>
    bool ScriptableImGui::ActionHelper(
        const char* label,
        AZStd::function<bool()> imguiAction,
        AZStd::function<void(const AZStd::string& /*pathToImGuiItem*/)> reportScriptableAction,
        AZStd::function<bool(ActionDataT /*scriptArg*/)> handleScriptedAction,
        bool shouldReportScriptableActionAfterAnyChange)
    {
        bool imResult = imguiAction();

        const AZStd::string pathToImGuiItem = MakeFullPath(label);

        if (ImGui::IsItemDeactivatedAfterEdit() || (shouldReportScriptableActionAfterAnyChange && imResult))
        {
            reportScriptableAction(pathToImGuiItem);
        }

        bool scriptResult = false;

        ActionItem actionItem = FindAndRemoveAction(pathToImGuiItem);
        bool foundAction = !AZStd::holds_alternative<InvalidActionItem>(actionItem);
        if (foundAction)
        {
            if (AZStd::holds_alternative<ActionDataT>(actionItem))
            {
                scriptResult = handleScriptedAction(AZStd::get<ActionDataT>(actionItem));
            }
            else
            {
                ReportScriptError(AZStd::string::format("Wrong data type used to set '%s'", pathToImGuiItem.c_str()).c_str());
            }
        }

        return imResult || scriptResult;
    }

    bool ScriptableImGui::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
    {
        PushNameContext(name);
        return ImGui::Begin(name, p_open, flags);
    }

    void ScriptableImGui::End()
    {
        ImGui::End();
        PopNameContext();
    }

    bool ScriptableImGui::Checkbox(const char* label, bool* v)
    {
        auto imguiAction = [&]()
        {
            return ImGui::Checkbox(label, v);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', %s)\n", pathToImGuiItem.c_str(), (*v ? "true" : "false"));
        };

        auto handleScriptedAction = [&](bool scriptArg)
        {
            if (*v != scriptArg)
            {
                *v = scriptArg;
                return true;
            }
            return false;
        };

        return ActionHelper<bool>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::Button(const char* label, const ImVec2& size_arg)
    {
        auto imguiAction = [&]()
        {
            return ImGui::Button(label, size_arg);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', true)\n", pathToImGuiItem.c_str());
        };

        auto handleScriptedAction = [](bool scriptArg)
        {
            return scriptArg;
        };

        return ActionHelper<bool>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::ListBox(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items)
    {
        auto imguiAction = [&]()
        {
            return ImGui::ListBox(label, current_item, items_getter, data, items_count, height_in_items);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            const char* itemText = nullptr;
            if (items_getter(data, *current_item, &itemText) && itemText)
            {
                AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', '%s')\n", pathToImGuiItem.c_str(), itemText);
            }
        };

        auto handleScriptedAction = [&](AZStd::string scriptArg)
        {
            for (int i = 0; i < items_count; ++i)
            {
                const char* itemText = nullptr;
                if (items_getter(data, i, &itemText) && itemText && scriptArg == itemText)
                {
                    *current_item = i;
                    return true;
                }
            }

            ReportScriptError(AZStd::string::format("List '%s' does not contain item '%s'", label, scriptArg.c_str()).c_str());
            return false;
        };

        return ActionHelper<AZStd::string>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
    {
        auto imguiAction = [&]()
        {
            return ImGui::Combo(label, current_item, items, items_count, height_in_items);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', '%s')\n", pathToImGuiItem.c_str(), items[*current_item]);
        };

        auto handleScriptedAction = [&](AZStd::string scriptArg)
        {
            for (int i = 0; i < items_count; ++i)
            {
                if (items[i] == scriptArg)
                {
                    *current_item = i;
                    return true;
                }
            }

            ReportScriptError(AZStd::string::format("Combo box '%s' does not contain item '%s'", label, scriptArg.c_str()).c_str());
            return false;
        };

        return ActionHelper<AZStd::string>(label, imguiAction, reportScriptableAction, handleScriptedAction,
            true /* It seems ImGui::Combo doesn't work with IsItemDeactivatedAfterChange() */);
    }

    bool ScriptableImGui::RadioButton(const char* label, int* v, int v_button)
    {
        auto imguiAction = [&]()
        {
            return ImGui::RadioButton(label, v, v_button);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', true)\n", pathToImGuiItem.c_str());
        };

        auto handleScriptedAction = [&](bool scriptArg)
        {
            if (scriptArg)
            {
                *v = v_button;
                return true;
            }
            return false;
        };

        return ActionHelper<bool>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
    {
        auto imguiAction = [&]()
        {
            return ImGui::SliderInt(label, v, v_min, v_max, format);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', %d)\n", pathToImGuiItem.c_str(), *v);
        };

        auto handleScriptedAction = [&](float scriptArg)
        {
            *v = aznumeric_cast<int>(scriptArg);
            return true;
        };

        return ActionHelper<float>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        auto imguiAction = [&]()
        {
            return ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', %f)\n", pathToImGuiItem.c_str(), *v);
        };

        auto handleScriptedAction = [&](float scriptArg)
        {
            *v = scriptArg;
            return true;
        };

        return ActionHelper<float>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        auto imguiAction = [&]()
        {
            return ImGui::SliderFloat2(label, v, v_min, v_max, format, flags);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', Vector2(%f, %f))\n", pathToImGuiItem.c_str(), v[0], v[1]);
        };

        auto handleScriptedAction = [&](AZ::Vector2 scriptArg)
        {
            v[0] = scriptArg.GetX();
            v[1] = scriptArg.GetY();
            return true;
        };

        return ActionHelper<AZ::Vector2>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    template <typename ImGuiActionType>
    bool ScriptableImGui::ThreeComponentHelper(const char* label, float v[3], ImGuiActionType& imguiAction)
    {
        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', Vector3(%f, %f, %f))\n", pathToImGuiItem.c_str(), v[0], v[1], v[2]);
        };

        auto handleScriptedAction = [&](AZ::Vector3 scriptArg)
        {
            v[0] = scriptArg.GetX();
            v[1] = scriptArg.GetY();
            v[2] = scriptArg.GetZ();
            return true;
        };

        return ActionHelper<AZ::Vector3>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        auto imguiAction = [&]()
        {
            return ImGui::SliderFloat3(label, v, v_min, v_max, format, flags);
        };

        return ThreeComponentHelper(label, v, imguiAction);
    }

    bool ScriptableImGui::ColorEdit3(const char* label, float v[3], ImGuiColorEditFlags flags)
    {
        auto imguiAction = [&]()
        {
            return ImGui::ColorEdit3(label, v, flags);
        };

        return ThreeComponentHelper(label, v, imguiAction);
    }

    bool ScriptableImGui::ColorPicker3(const char* label, float v[3], ImGuiColorEditFlags flags)
    {
        auto imguiAction = [&]()
        {
            return ImGui::ColorPicker3(label, v, flags);
        };

        return ThreeComponentHelper(label, v, imguiAction);
    }

    bool ScriptableImGui::SliderAngle(const char* label, float* v, float v_min, float v_max, const char* format)
    {
        auto imguiAction = [&]()
        {
            return ImGui::SliderAngle(label, v, v_min, v_max, format);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', %f)\n", pathToImGuiItem.c_str(), *v);
        };

        auto handleScriptedAction = [&](float scriptArg)
        {
            *v = scriptArg;
            return true;
        };

        return ActionHelper<float>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size)
    {
        auto imguiAction = [&]()
        {
            return ImGui::Selectable(label, selected, flags, size);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            // The "selected" value that's passed determines if the selectable is *currently* selected, and clicking the selectable toggles its state.
            // So when someone clicks the selectable to change its state, we need to report the opposite of what the original state was.
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', %s)\n", pathToImGuiItem.c_str(), (selected ? "false" : "true"));
        };

        auto handleScriptedAction = [&](bool scriptArg)
        {
            return scriptArg != selected;
        };

        return ActionHelper<bool>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size)
    {
        auto imguiAction = [&]()
        {
            return ImGui::Selectable(label, p_selected, flags, size);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            // The "selected" value that's passed determines if the selectable is *currently* selected, and clicking the selectable toggles its state.
            // So when someone clicks the selectable to change its state, we need to report the opposite of what the original state was.
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', %s)\n", pathToImGuiItem.c_str(), (*p_selected ? "false" : "true"));
        };

        auto handleScriptedAction = [&](bool scriptArg)
        {
            if (scriptArg != *p_selected)
            {
                *p_selected = scriptArg;
                return true;
            }
            return false;
        };

        return ActionHelper<bool>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }

    bool ScriptableImGui::TreeNodeEx(const char* label, ImGuiSelectableFlags flags)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);

        if (ImGui::TreeNodeEx(label, flags))
        {
            PushNameContext(label);
            return true;
        }

        return false;
    }

    void ScriptableImGui::TreePop()
    {
        ImGui::TreePop();
        PopNameContext();
    }

    bool ScriptableImGui::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
    {
        AZ_DEBUG_STATIC_MEMEBER(instance, s_instance);

        const AZStd::string pathToImGuiItem = MakeFullPath(label);

        if (ImGui::BeginCombo(label, preview_value, flags))
        {
            PushNameContext(label);
            return true;
        }
        else if (!s_instance->m_scriptedActions.empty())
        {
            // If a script is running, we return true so that imgui controls inside the combo box are checked.
            // We also have to set a flag to prevent ScriptableImGui::EndCombo() from calling ImGui::EndCombo()
            // because that is only allowed when ImGui::BeginCombo() returns true.
            s_instance->m_isInScriptedComboPopup = true;
            PushNameContext(label);
            return true;
        }

        return false;
    }

    void ScriptableImGui::EndCombo()
    {
        if (s_instance->m_isInScriptedComboPopup)
        {
            s_instance->m_isInScriptedComboPopup = false;

            // ScriptableImGui::BeginCombo() returned true even though ImGui::BeginCombo() didn't, so we aren't allowed
            // to call ImGui::EndCombo() here.
        }
        else
        {
            ImGui::EndCombo();
        }

        PopNameContext();
    }

    bool ScriptableImGui::BeginMenu(const char* label, bool enabled)
    {
        // We don't use ActionHelper here because BeginMenu has to do things a bit different since there is a persistent popup.
        // It has to run the script code before the ImGui code, and if there is a scripted action then force the menu to open.
        // Also, we don't include a "scriptResult", just the "imResult", because we need to ensure that ImGui is in the actual
        // state we are reporting back to the caller. Otherwise the internal state of ImGui could become invalid and crash.

        const AZStd::string pathToImGuiItem = MakeFullPath(label);
        
        ActionItem actionItem = FindAndRemoveAction(pathToImGuiItem);
        bool foundAction = !AZStd::holds_alternative<InvalidActionItem>(actionItem);
        if (foundAction)
        {
            if (AZStd::holds_alternative<bool>(actionItem))
            {
                if (AZStd::get<bool>(actionItem))
                {
                    // Here we force the menu to open before arriving at ImGui::BeginMenu 
                    ImGui::OpenPopup(label);
                }
            }
            else
            {
                ReportScriptError(AZStd::string::format("Wrong data type used to set '%s'", label).c_str());
            }
        }

        bool wasPopupOpen = ImGui::IsPopupOpen(label);

        bool isPopupOpen = ImGui::BeginMenu(label, enabled);

        if (isPopupOpen)
        {
            PushNameContext(label);

            if (!wasPopupOpen && isPopupOpen)
            {
                AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', true)\n", pathToImGuiItem.c_str());
            }
        }

        return isPopupOpen;
    }

    void ScriptableImGui::EndMenu()
    {
        ImGui::EndMenu();
        PopNameContext();
    }

    bool ScriptableImGui::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled)
    {
        auto imguiAction = [&]()
        {
            return ImGui::MenuItem(label, shortcut, selected, enabled);
        };

        auto reportScriptableAction = [&](const AZStd::string& pathToImGuiItem)
        {
            AZ_TracePrintf("ScriptAutomation", "SetImguiValue('%s', true)\n", pathToImGuiItem.c_str());
        };

        auto handleScriptedAction = [](bool scriptArg)
        {
            return scriptArg;
        };

        return ActionHelper<bool>(label, imguiAction, reportScriptableAction, handleScriptedAction);
    }


} // namespace ScriptAutomation
