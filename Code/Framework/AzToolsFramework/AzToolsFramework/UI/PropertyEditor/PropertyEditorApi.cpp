/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyEditorAPI.h"
#include <QDoubleSpinBox>
#include <QCoreApplication>

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/DynamicSerializableField.h>

#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace AzToolsFramework
{
    // Registry of translation contexts for TranslatePropertyString.
    // Modules/Gems can register additional contexts at startup via RegisterTranslationContext().
    static AZStd::vector<AZStd::string>& GetRegisteredContexts()
    {
        static AZStd::vector<AZStd::string> s_contexts = {
            // Core framework contexts (always present)
            "AzCore",
            "AzFramework",
            "AzToolsFramework",
            "AtomToolsFramework",
            "Editor",
            // Gem contexts (auto-populated, or registered at runtime)
            "Achievements", "AssetValidation", "Atom::Asset", "Atom::Bootstrap",
            "Atom::Feature", "Atom::RHI", "Atom::RPI", "AtomBridge", "AtomFont",
            "AtomImGuiTools", "AtomLyIntegration", "AtomRenderOptions",
            "AtomTressFX", "AtomViewportDisplayIcons", "AtomViewportDisplayInfo",
            "AudioSystem", "AZ::Render", "BarrierInput", "Camera", "CameraFramework",
            "DebugDraw", "DiffuseProbeGrid", "EditorModeFeedback",
            "EditorPythonBindings", "EMotionFX", "ExpressionEvaluation",
            "FastNoiseGem", "GameState", "Gestures", "GradientSignal",
            "GraphCanvas", "GraphModel", "InAppPurchases", "LandscapeCanvas",
            "LmbrCentral", "LocalUser", "LyShine", "LyShineExamples",
            "Maestro", "Meshlets", "MessagePopup", "Microphone", "MiniAudio",
            "MotionMatching", "Multiplayer", "MultiplayerCompression", "NvCloth",
            "OpenParticleSystem", "PhysX", "Presence", "Profiler",
            "RecastNavigation", "RemoteTools", "SaveData",
            "SceneLoggingExample", "SceneProcessing", "ScriptAutomation",
            "ScriptCanvas", "ScriptCanvasDeveloper", "ScriptCanvasMultiplayer",
            "ScriptCanvasPhysics", "ScriptCanvasTesting", "ScriptEvents",
            "SkyAtmosphere", "StartingPointInput", "Streamer", "SurfaceData",
            "Terrain", "TextureAtlasNamespace", "TickBusOrderViewer",
            "Vegetation", "VideoPlaybackFramework", "VirtualGamepad", "WhiteBox",
            // Editor preferences pages
            "EditorPreferencesPageAWS", "EditorPreferencesPageFiles",
            "EditorPreferencesPageGeneral", "EditorPreferencesPageViewportCamera",
            "EditorPreferencesPageViewportDebug", "EditorPreferencesPageViewportGeneral",
            "EditorPreferencesPageViewportManipulator",
            // View pane names
            "LyViewPane",
            // Transform selection
            "EditorTransformComponentSelection",
            // Component-specific contexts (used in right-click menus, undo labels, etc.)
            "TransformComponent",
            // JSON property data contexts (strings extracted from JSON by Translation.cmake)
            "MaterialInputs",
            // Reflected property contexts (used by EditContext in ProjectSettingsTool, etc.)
            "ReflectedPropertyEditor",
        };
        return s_contexts;
    }

    void ClearTranslationContexts()
    {
        AZStd::vector<AZStd::string> empty;
        GetRegisteredContexts().swap(empty);
    }

    void RegisterTranslationContext(const AZStd::string& contextName)
    {
        auto& contexts = GetRegisteredContexts();
        for (const auto& existing : contexts)
        {
            if (existing == contextName)
            {
                return; // Already registered
            }
        }
        contexts.push_back(contextName);
    }

    // Helper function: attempt to translate property editor strings from EditContext.
    // Searches all registered translation contexts. Qt's QCoreApplication::translate()
    // queries all installed QTranslator objects for each context, so we try each
    // registered context until a translation is found.
    AZStd::string TranslatePropertyString(AZStd::string_view sourceText, AZStd::string_view context)
    {
        if (sourceText.empty())
        {
            return AZStd::string{};
        }

        // QCoreApplication::translate requires null-terminated strings.
        // string_view from string literals and .c_str()/.data() is already null-terminated,
        // but we use QByteArray to guarantee it for the general case.
        QByteArray sourceUtf8(sourceText.data(), static_cast<int>(sourceText.size()));

        // If a specific context is provided, try it first (fastest path).
        if (!context.empty())
        {
            QByteArray ctxUtf8(context.data(), static_cast<int>(context.size()));
            QString translated = QCoreApplication::translate(ctxUtf8.constData(), sourceUtf8.constData());
            if (translated != QString::fromUtf8(sourceUtf8))
            {
                return translated.toUtf8().constData();
            }
        }

        // Search all registered contexts
        QString source = QString::fromUtf8(sourceUtf8);
        for (const auto& ctx : GetRegisteredContexts())
        {
            QString translated = QCoreApplication::translate(ctx.c_str(), sourceUtf8.constData());
            if (translated != source)
            {
                return translated.toUtf8().constData();
            }
        }

        // No translation found, return the original string.
        return AZStd::string(sourceText);
    }

    PropertyHandlerBase::PropertyHandlerBase()
    {
    }

    PropertyHandlerBase::~PropertyHandlerBase()
    {
    }

    // default impl deletes.
    void PropertyHandlerBase::DestroyGUI(QWidget* pTarget)
    {
        delete pTarget;
    }

    bool NodeMatchesFilter(const InstanceDataNode& node, const char* filter)
    {
        if (!filter || filter[0] == '\0')
        {
            return true;
        }

        if (node.GetElementEditMetadata() && node.GetElementEditMetadata()->m_name)
        {
            // Check if we match the filter
            if (AzFramework::StringFunc::Find(node.GetElementEditMetadata()->m_name, filter) != AZStd::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    bool NodeGroupMatchesFilter(const InstanceDataNode& node, const char* filter)
    {
        if (!filter || filter[0] == '\0')
        {
            return true;
        }

        if (node.GetGroupElementMetadata() && node.GetGroupElementMetadata()->m_description)
        {
            // Check if we match the filter
            if (AzFramework::StringFunc::Find(node.GetGroupElementMetadata()->m_description, filter) != AZStd::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------

    bool HasAnyVisibleChildren(const InstanceDataNode& node, bool isSlicePushUI)
    {
        // Seed the list.
        AZStd::list<InstanceDataNode> nodeList(node.GetChildren().begin(), node.GetChildren().end());

        for (auto& child : nodeList)
        {
            NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(child, isSlicePushUI);
            if (visibility == NodeDisplayVisibility::Visible)
            {
                return true;
            }

            // Queue the rest of the list.
            // This is creates breadth-first traversal.
            nodeList.insert(nodeList.end(), child.GetChildren().begin(), child.GetChildren().end());
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    NodeDisplayVisibility CalculateNodeDisplayVisibility(const InstanceDataNode& node, bool isSlicePushUI)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        NodeDisplayVisibility visibility = NodeDisplayVisibility::NotVisible;

        // If parent is a dynamic serializable field with edit reflection, default to visible.
        if (node.GetElementMetadata() && 0 != (node.GetElementMetadata()->m_flags & AZ::SerializeContext::ClassElement::FLG_DYNAMIC_FIELD))
        {
            if (node.GetParent() && node.GetParent()->GetElementEditMetadata())
            {
                visibility = NodeDisplayVisibility::Visible;
            }
        }

        // Show UI Elements by default
        if (node.GetElementMetadata() && 0 != (node.GetElementMetadata()->m_flags & AZ::SerializeContext::ClassElement::FLG_UI_ELEMENT))
        {
            visibility = NodeDisplayVisibility::Visible;
        }

        // Use class meta data as opposed to parent's reflection data if this is a root node or a container element.
        if (visibility == NodeDisplayVisibility::NotVisible && (!node.GetParent() || node.GetParent()->GetClassMetadata()->m_container))
        {
            if (node.GetClassMetadata() && node.GetClassMetadata()->m_editData)
            {
                visibility = NodeDisplayVisibility::Visible;
            }
        }

        // Use class meta data as opposed to parent's reflection data if this is a base class element,
        // which isn't explicitly reflected by the containing class.
        if ((visibility == NodeDisplayVisibility::NotVisible && node.GetElementMetadata()) &&
            (node.GetElementMetadata()->m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS))
        {
            if (node.GetClassMetadata() && node.GetClassMetadata()->m_editData)
            {
                visibility = NodeDisplayVisibility::ShowChildrenOnly;
            }
        }

        // Child nodes must have edit data in their parent's reflection.
        if (visibility == NodeDisplayVisibility::NotVisible && node.GetElementEditMetadata())
        {
            visibility = NodeDisplayVisibility::Visible;
        }

        // Finally, check against reflection attributes.
        if (visibility == NodeDisplayVisibility::Visible)
        {
            const AZ::Crc32 visibilityAttribute = ResolveVisibilityAttribute(node);

            if (visibilityAttribute == AZ::Edit::PropertyVisibility::Hide)
            {
                visibility = NodeDisplayVisibility::NotVisible;
            }
            else if (visibilityAttribute == AZ::Edit::PropertyVisibility::Show)
            {
                visibility = NodeDisplayVisibility::Visible;
            }
            else if (visibilityAttribute == AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            {
                visibility = NodeDisplayVisibility::ShowChildrenOnly;
            }
            else if (visibilityAttribute == AZ::Edit::PropertyVisibility::HideChildren)
            {
                visibility = NodeDisplayVisibility::HideChildren;
            }
        }

        if (isSlicePushUI)
        {
            // Components should always appear, even if they're not exposed in the inspector UI.
            if (visibility != AzToolsFramework::NodeDisplayVisibility::Visible)
            {
                const AZ::SerializeContext::ClassData* classData = node.GetClassMetadata();
                if (classData && classData->m_azRtti && classData->m_azRtti->IsTypeOf(azrtti_typeid<AZ::Component>()))
                {
                    visibility = AzToolsFramework::NodeDisplayVisibility::Visible;
                }
            }
        }

        return visibility;
    }

    //-----------------------------------------------------------------------------
    AZStd::string GetNodeDisplayName(const AzToolsFramework::InstanceDataNode& node)
    {
        // Introspect template for generic component wrappers.
        if (node.GetClassMetadata() &&
            node.GetClassMetadata()->m_typeId == AZ::AzTypeInfo<AzToolsFramework::Components::GenericComponentWrapper>::Uuid())
        {
            Components::GenericComponentWrapper* componentWrapper = nullptr;
            if (node.GetNumInstances() > 0)
            {
                componentWrapper = static_cast<Components::GenericComponentWrapper*>(node.FirstInstance());
            }
            else
            {
                auto comparisonNode = node.GetComparisonNode();
                if (comparisonNode && comparisonNode->GetNumInstances() > 0)
                {
                    componentWrapper = static_cast<Components::GenericComponentWrapper*>(comparisonNode->FirstInstance());
                }
            }

            if (componentWrapper)
            {
                if (componentWrapper->GetTemplate())
                {
                    return TranslatePropertyString(componentWrapper->GetDisplayName());
                }

                return TranslatePropertyString(QT_TRANSLATE_NOOP("AzToolsFramework", "<empty component>"));
            }
        }

        // Otherwise, search for a valid display name override in the node edit, class, and element metadata.
        // Try to translate the display name from EditContext
        if (const auto metadata = node.GetElementEditMetadata(); metadata && metadata->m_name)
        {
            return TranslatePropertyString(metadata->m_name);
        }

        if (const auto metadata = node.GetClassMetadata(); metadata && metadata->m_editData && metadata->m_editData->m_name)
        {
            return TranslatePropertyString(metadata->m_editData->m_name);
        }

        if (const auto metadata = node.GetElementMetadata(); metadata && metadata->m_name && metadata->m_nameCrc != AZ_CRC_CE("element"))
        {
            return TranslatePropertyString(metadata->m_name);
        }

        if (const auto metadata = node.GetClassMetadata(); metadata && metadata->m_name)
        {
            return TranslatePropertyString(metadata->m_name);
        }

        return AZStd::string{};
    }

    bool ReadVisibilityAttribute(void* instance, AZ::Edit::Attribute* attr, AZ::Crc32& visibility)
    {
        PropertyAttributeReader reader(instance, attr);
        if (reader.Read<AZ::Crc32>(visibility))
        {
            return true;
        }
        else
        {
            AZ::u32 valueCrc = 0;
            if (reader.Read<AZ::u32>(valueCrc))
            {
                // Assume crc returned as u32.
                visibility = AZ::Crc32(valueCrc);

                // Support 0|1 return values.
                if (valueCrc == 0)
                {
                    visibility = AZ::Edit::PropertyVisibility::Hide;
                }
                else if (valueCrc == 1)
                {
                    visibility = AZ::Edit::PropertyVisibility::Show;
                }
                return true;
            }

            bool visible = false;
            if (reader.Read<bool>(visible))
            {
                visibility = visible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
                return true;
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    AZ::Crc32 ResolveVisibilityAttribute(const InstanceDataNode& node)
    {
        // First check the data element metadata in the reflecting class
        if (const auto* editElement = node.GetElementEditMetadata())
        {
            for (size_t attrIndex = 0; attrIndex < editElement->m_attributes.size(); ++attrIndex)
            {
                const auto& attrPair = editElement->m_attributes[attrIndex];
                // Ensure the visibility attribute isn't intended for child elements
                if (attrPair.first == AZ::Edit::Attributes::Visibility && !attrPair.second->m_describesChildren)
                {
                    if (auto* parentInstance = node.GetParent())
                    {
                        AZ::Crc32 visibility;
                        for (size_t instIndex = 0; instIndex < parentInstance->GetNumInstances(); ++instIndex)
                        {
                            if (ReadVisibilityAttribute(parentInstance->GetInstance(instIndex), attrPair.second, visibility))
                            {
                                return visibility;
                            }
                        }
                    }
                }
            }
        }

        // Check for any element attributes on the parent container (if there is one)
        if (node.GetParent() && node.GetParent()->GetClassMetadata()->m_container)
        {
            if (const auto* editElement = node.GetParent()->GetElementEditMetadata())
            {
                for (size_t attrIndex = 0; attrIndex < editElement->m_attributes.size(); ++attrIndex)
                {
                    const auto& attrPair = editElement->m_attributes[attrIndex];
                    // Parent attributes must describe children to apply here
                    if (attrPair.first == AZ::Edit::Attributes::Visibility && attrPair.second->m_describesChildren)
                    {
                        if (auto* parentInstance = node.GetParent())
                        {
                            AZ::Crc32 visibility;
                            for (size_t instIndex = 0; instIndex < parentInstance->GetNumInstances(); ++instIndex)
                            {
                                if (ReadVisibilityAttribute(parentInstance->GetInstance(instIndex), attrPair.second, visibility))
                                {
                                    return visibility;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Check class editor metadata
        if (const auto* classElement = node.GetClassMetadata())
        {
            if (const auto* editElement = classElement->m_editData)
            {
                if (!editElement->m_elements.empty())
                {
                    const auto& element = *editElement->m_elements.begin();
                    for (const auto& attrPair : element.m_attributes)
                    {
                        if (attrPair.first == AZ::Edit::Attributes::Visibility)
                        {
                            AZ::Crc32 visibility;
                            for (size_t instIndex = 0; instIndex < node.GetNumInstances(); ++instIndex)
                            {
                                if (ReadVisibilityAttribute(node.GetInstance(instIndex), attrPair.second, visibility))
                                {
                                    return visibility;
                                }
                            }
                        }
                    }
                }
            }
        }

        // No one said no, show by default
        return AZ::Edit::PropertyVisibility::Show;
    }

    void OnEntityComponentPropertyChanged(AZ::EntityId entityId, AZ::ComponentId componentId)
    {
        PropertyEditorEntityChangeNotificationBus::Event(
            entityId, &PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged,
            componentId);
    }

    void OnEntityComponentPropertyChanged(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        OnEntityComponentPropertyChanged(
            entityComponentIdPair.GetEntityId(), entityComponentIdPair.GetComponentId());
    }

} // namespace AzToolsFramework
