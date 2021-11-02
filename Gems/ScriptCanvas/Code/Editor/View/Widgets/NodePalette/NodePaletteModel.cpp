/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>

#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeUtils.h>
#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>

#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

#include <ScriptCanvas/Data/Traits.h>

namespace
{
    // Various Helper Methods
    bool IsDeprecated(const AZ::AttributeArray& attributes)
    {
        bool isDeprecated{};

        if (auto isDeprecatedAttributePtr = AZ::FindAttribute(AZ::Script::Attributes::Deprecated, attributes))
        {
            AZ::AttributeReader(nullptr, isDeprecatedAttributePtr).Read<bool>(isDeprecated);
        }

        return isDeprecated;
    }

    bool ShouldExcludeFromNodeList(const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>* excludeAttributeData, const AZ::Uuid& typeId)
    {
        if (excludeAttributeData)
        {
            AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::List | AZ::Script::Attributes::ExcludeFlags::ListOnly;

            if (typeId == AzToolsFramework::Components::EditorComponentBase::TYPEINFO_Uuid())
            {
                return true;
            }

            return (static_cast<AZ::u64>(excludeAttributeData->Get(nullptr)) & exclusionFlags) != 0; // warning C4800: 'AZ::u64': forcing value to bool 'true' or 'false' (performance warning)
        }

        return false;
    }

    bool HasExcludeFromNodeListAttribute(const AZ::SerializeContext* serializeContext, const AZ::Uuid& typeId)
    {
        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);
        if (classData && classData->m_editData)
        {
            if (auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto excludeAttribute = editorElementData->FindAttribute(AZ::Script::Attributes::ExcludeFrom))
                {
                    auto excludeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(excludeAttribute);
                    return excludeAttributeData && ShouldExcludeFromNodeList(excludeAttributeData, typeId);
                }
            }
        }

        return false;
    }

    bool MethodHasAttribute(const AZ::BehaviorMethod* method, AZ::Crc32 attribute)
    {
        return AZ::FindAttribute(attribute, method->m_attributes) != nullptr; // warning C4800: 'AZ::Attribute *': forcing value to bool 'true' or 'false' (performance warning)
    }

    // Checks for and returns the Category attribute from an AZ::AttributeArray
    AZStd::string GetCategoryPath(const AZ::AttributeArray& attributes, const AZ::BehaviorContext& behaviorContext)
    {
        AZStd::string retVal;
        AZ::Attribute* categoryAttribute = AZ::FindAttribute(AZ::Script::Attributes::Category, attributes);

        if (categoryAttribute)
        {
            AZ::AttributeReader(nullptr, categoryAttribute).Read<AZStd::string>(retVal, behaviorContext);
        }

        return retVal;
    }

    bool IsExplicitOverload(const AZ::BehaviorMethod& method)
    {
        return AZ::FindAttribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, method.m_attributes) != nullptr;
    }

    void RegisterMethod
        ( ScriptCanvasEditor::NodePaletteModel& nodePaletteModel
        , const AZ::BehaviorContext& behaviorContext
        , const AZStd::string& categoryPath
        , const AZ::BehaviorClass* behaviorClass
        , const AZStd::string& name
        , const AZ::BehaviorMethod& method
        , ScriptCanvas::PropertyStatus propertyStatus
        , bool isOverloaded)
    {
        if (IsDeprecated(method.m_attributes))
        {
            return;
        }

        if (behaviorClass && !isOverloaded)
        {
            auto excludeMethodAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, method.m_attributes));
            if (ShouldExcludeFromNodeList(excludeMethodAttributeData, behaviorClass->m_azRtti ? behaviorClass->m_azRtti->GetTypeId() : behaviorClass->m_typeId))
            {
                return;
            }
        }

        const auto isExposableOutcome = ScriptCanvas::IsExposable(method);
        if (!isExposableOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false, "Unable to expose method: %s to ScriptCanvas because: %s", method.m_name.data(), isExposableOutcome.GetError().data());
            return;
        }

        // If the reflected method returns an AZ::Event, reflect it to the SerializeContext
        if (AZ::MethodReturnsAzEventByReferenceOrPointer(method))
        {
            const AZ::BehaviorParameter* resultParameter = method.GetResult();
            ScriptCanvas::ReflectEventTypeOnDemand(resultParameter->m_typeId, resultParameter->m_name, resultParameter->m_azRtti);
        }

        nodePaletteModel.RegisterClassNode(categoryPath, behaviorClass ? behaviorClass->m_name : "", name, &method, &behaviorContext, propertyStatus, isOverloaded);
    }

    void RegisterGlobalMethod(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel, const AZ::BehaviorContext& behaviorContext,
        const AZ::BehaviorMethod& behaviorMethod)
    {
        const auto isExposableOutcome = ScriptCanvas::IsExposable(behaviorMethod);
        if (!isExposableOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false, "Unable to expose method: %s to ScriptCanvas because: %s",
                behaviorMethod.m_name.c_str(), isExposableOutcome.GetError().data());
            return;
        }

        if (!AZ::Internal::IsInScope(behaviorMethod.m_attributes, AZ::Script::Attributes::ScopeFlags::Common))
        {
            return; // skip this method
        }

        // If the reflected method returns an AZ::Event, reflect it to the SerializeContext
        if (AZ::MethodReturnsAzEventByReferenceOrPointer(behaviorMethod))
        {
            const AZ::BehaviorParameter* resultParameter = behaviorMethod.GetResult();
            ScriptCanvas::ReflectEventTypeOnDemand(resultParameter->m_typeId, resultParameter->m_name, resultParameter->m_azRtti);

        }
        nodePaletteModel.RegisterMethodNode(behaviorContext, behaviorMethod);
    }

    //! Retrieve the list of EBuses t hat should not be exposed in the ScriptCanvasEditor Node Palette
    AZStd::unordered_set<AZ::Crc32> GetEBusExcludeSet(const AZ::BehaviorContext& behaviorContext)
    {
        // We will skip buses that are ONLY registered on classes that derive from EditorComponentBase,
        // because they don't have a runtime implementation. Buses such as the TransformComponent which
        // is implemented by both an EditorComponentBase derived class and a Component derived class
        // will still appear
        AZStd::unordered_set<AZ::Crc32> skipBuses;
        AZStd::unordered_set<AZ::Crc32> potentialSkipBuses;
        AZStd::unordered_set<AZ::Crc32> nonSkipBuses;
        for (const auto& classIter : behaviorContext.m_classes)
        {
            const AZ::BehaviorClass* behaviorClass = classIter.second;

            if (IsDeprecated(behaviorClass->m_attributes))
            {
                continue;
            }

            // Only bind Behavior Classes marked with the Scope type of Launcher
            if (!AZ::Internal::IsInScope(behaviorClass->m_attributes, AZ::Script::Attributes::ScopeFlags::Launcher))
            {
                continue; // skip this class
            }

            // Check for "ExcludeFrom" attribute for ScriptCanvas
            auto excludeClassAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(
                AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));

            // We don't want to show any components, since there isn't anything we can do with them
            // from ScriptCanvas since we use buses to communicate to everything.
            if (ShouldExcludeFromNodeList(excludeClassAttributeData, behaviorClass->m_azRtti ? behaviorClass->m_azRtti->GetTypeId() : behaviorClass->m_typeId))
            {
                for (const auto& requestBus : behaviorClass->m_requestBuses)
                {
                    skipBuses.insert(AZ::Crc32(requestBus.c_str()));
                }

                continue;
            }

            auto baseClass = AZStd::find(behaviorClass->m_baseClasses.begin(),
                behaviorClass->m_baseClasses.end(),
                AzToolsFramework::Components::EditorComponentBase::TYPEINFO_Uuid());

            if (baseClass != behaviorClass->m_baseClasses.end())
            {
                for (const auto& requestBus : behaviorClass->m_requestBuses)
                {
                    potentialSkipBuses.insert(AZ::Crc32(requestBus.c_str()));
                }
            }
            // If the Ebus does not inherit from EditorComponentBase then do not skip it
            else
            {
                for (const auto& requestBus : behaviorClass->m_requestBuses)
                {
                    nonSkipBuses.insert(AZ::Crc32(requestBus.c_str()));
                }
            }
        }

        // Add buses which are not on the non-skip list to the skipBuses set
        for (auto potentialSkipBus : potentialSkipBuses)
        {
            if (nonSkipBuses.find(potentialSkipBus) == nonSkipBuses.end())
            {
                skipBuses.insert(potentialSkipBus);
            }
        }

        return skipBuses;
    }

    //! Register all nodes populated into the ScriptCanvas NodeRegistry for each class derived
    //! from the ScriptCanvas LibraryDefinition class
    void PopulateScriptCanvasDerivedNodes(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::SerializeContext& serializeContext)
    {
        // Get all the types.
        auto EnumerateLibraryDefintionNodes = [&nodePaletteModel, &serializeContext](
            const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
        {
            ScriptCanvasEditor::CategoryInformation categoryInfo;

            AZStd::string categoryPath = classData->m_editData ? classData->m_editData->m_name : classData->m_name;

            if (classData->m_editData)
            {
                auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                if (editorElementData)
                {
                    if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Category))
                    {
                        if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                        {
                            categoryPath = categoryAttributeData->Get(nullptr);
                        }
                    }

                    if (auto categoryStyleAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                    {
                        if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryStyleAttribute))
                        {
                            categoryInfo.m_styleOverride = categoryAttributeData->Get(nullptr);
                        }
                    }

                    if (auto titlePaletteAttribute = editorElementData->FindAttribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride))
                    {
                        if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(titlePaletteAttribute))
                        {
                            categoryInfo.m_paletteOverride = categoryAttributeData->Get(nullptr);
                        }
                    }
                }
            }

            nodePaletteModel.RegisterCategoryInformation(categoryPath, categoryInfo);

            // Children
            for (auto& node : ScriptCanvas::Library::LibraryDefinition::GetNodes(classData->m_typeId))
            {
                if (HasExcludeFromNodeListAttribute(&serializeContext, node.first))
                {
                    continue;
                }

                // Pass in the associated class data so we can do more intensive lookups?
                const AZ::SerializeContext::ClassData* nodeClassData = serializeContext.FindClassData(node.first);

                if (nodeClassData == nullptr)
                {
                    continue;
                }

                // Skip over some of our more dynamic nodes that we want to populate using different means
                else if (nodeClassData->m_azRtti && nodeClassData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::GetVariableNode>())
                {
                    continue;
                }
                else if (nodeClassData->m_azRtti && nodeClassData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::SetVariableNode>())
                {
                    continue;
                }
                else
                {
                    nodePaletteModel.RegisterCustomNode(categoryPath, node.first, node.second, nodeClassData);
                }
            }

            return true;
        };

        const AZ::TypeId& libraryDefTypeId = azrtti_typeid<ScriptCanvas::Library::LibraryDefinition>();
        serializeContext.EnumerateDerived(EnumerateLibraryDefintionNodes, libraryDefTypeId, libraryDefTypeId);
    }

    void PopulateVariablePalette()
    {
        auto dataRegistry = ScriptCanvas::GetDataRegistry();

        for (auto& type : dataRegistry->m_creatableTypes)
        {
            if (!type.second.m_isTransient)
            {
                ScriptCanvasEditor::VariablePaletteRequestBus::Broadcast(&ScriptCanvasEditor::VariablePaletteRequests::RegisterVariableType, type.first);
            }
        }
    }

    void PopulateBehaviorContextGlobalMethods(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::BehaviorContext& behaviorContext)
    {
        // BehaviorMethods are not associated with a class
        // therefore the Uuid is set to Null
        const AZ::Uuid behaviorMethodUuid = AZ::Uuid::CreateNull();
        for (const auto& [methodName, behaviorMethod] : behaviorContext.m_methods)
        {
            // Skip behavior methods that are deprecated
            if (behaviorMethod == nullptr || IsDeprecated(behaviorMethod->m_attributes))
            {
                continue;
            }

            // Check for "ExcludeFrom" attribute for ScriptCanvas
            auto excludeMethodAttributeData = azrtti_cast<const AZ::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(
                AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorMethod->m_attributes));


            if (ShouldExcludeFromNodeList(excludeMethodAttributeData, behaviorMethodUuid))
            {
                continue;
            }

            RegisterGlobalMethod(nodePaletteModel, behaviorContext, *behaviorMethod);
        }
    }

    //! Iterates over all Properties directly reflected to the BehaviorContext instance
    //! and registers there Getter/Setter methods to the NodePaletteModel
    void PopulateBehaviorContextGlobalProperties(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::BehaviorContext& behaviorContext)
    {
        const AZ::Uuid behaviorMethodUuid = AZ::Uuid::CreateNull();
        for (const auto& [propertyName, behaviorProperty] : behaviorContext.m_properties)
        {
            // Skip behavior properties that are deprecated
            if (behaviorProperty == nullptr || IsDeprecated(behaviorProperty->m_attributes))
            {
                continue;
            }

            // Check for "ExcludeFrom" attribute for ScriptCanvas
            auto excludePropertyAttributeData = azrtti_cast<const AZ::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(
                AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorProperty->m_attributes));


            if (ShouldExcludeFromNodeList(excludePropertyAttributeData, behaviorMethodUuid))
            {
                continue;
            }

            if (behaviorProperty->m_getter && !behaviorProperty->m_setter)
            {
                nodePaletteModel.RegisterGlobalConstant(behaviorContext, behaviorProperty , *behaviorProperty->m_getter);
            }
            else
            {
                if (behaviorProperty->m_getter)
                {
                    RegisterGlobalMethod(nodePaletteModel, behaviorContext, *behaviorProperty->m_getter);
                }

                if (behaviorProperty->m_setter)
                {
                    RegisterGlobalMethod(nodePaletteModel, behaviorContext, *behaviorProperty->m_setter);
                }
            }

        }
    }

    void PopulateBehaviorContextClassMethods(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::BehaviorContext& behaviorContext)
    {
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        for (const auto& classIter : behaviorContext.m_classes)
        {
            const AZ::BehaviorClass* behaviorClass = classIter.second;

            if (IsDeprecated(behaviorClass->m_attributes))
            {
                continue;
            }

            if (auto excludeFromPointer = AZ::FindAttribute(AZ::ScriptCanvasAttributes::Internal::ImplementedAsNodeGeneric, behaviorClass->m_attributes))
            {
                continue;
            }

            if (auto excludeFromPointer = AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes))
            {
                AZ::Script::Attributes::ExcludeFlags excludeFlags{};
                AZ::AttributeReader(nullptr, excludeFromPointer).Read<AZ::Script::Attributes::ExcludeFlags>(excludeFlags);

                if ((excludeFlags & (AZ::Script::Attributes::ExcludeFlags::List | AZ::Script::Attributes::ExcludeFlags::ListOnly)) != 0)
                {
                    continue;
                }
            }

            if (!AZ::Internal::IsInScope(behaviorClass->m_attributes, AZ::Script::Attributes::ScopeFlags::Launcher))
            {
                continue;
            }

            // Objects and Object methods
            {
                AZStd::string categoryPath;

                GraphCanvas::TranslationKey key;
                key << "BehaviorClass" << behaviorClass->m_name.c_str() << "details";

                GraphCanvas::TranslationRequests::Details details;
                GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

                categoryPath = details.Category;

                if (categoryPath.empty())
                {
                    categoryPath = GetCategoryPath(behaviorClass->m_attributes, behaviorContext);
                }

                auto dataRegistry = ScriptCanvas::GetDataRegistry();
                ScriptCanvas::Data::Type type = dataRegistry->m_typeIdTraitMap[ScriptCanvas::Data::eType::BehaviorContextObject].m_dataTraits.GetSCType(behaviorClass->m_typeId);

                if (type.IsValid())
                {
                    if (dataRegistry->m_creatableTypes.contains(type))
                    {
                        ScriptCanvasEditor::VariablePaletteRequestBus::Broadcast(&ScriptCanvasEditor::VariablePaletteRequests::RegisterVariableType, type);
                    }
                }

                AZStd::string classNamePretty(classIter.first);

                AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes);

                if (prettyNameAttribute)
                {
                    AZ::AttributeReader(nullptr, prettyNameAttribute).Read<AZStd::string>(classNamePretty, behaviorContext);
                }

                if (categoryPath.empty())
                {
                    if (classNamePretty.empty())
                    {
                        categoryPath = classNamePretty;
                    }
                    else
                    {
                        categoryPath = "Other";
                    }
                }

                categoryPath.append("/");

                if (details.Name.empty())
                {
                    categoryPath.append(classNamePretty.c_str());
                }
                else
                {
                    categoryPath.append(details.Name.c_str());
                }

                for (auto property : behaviorClass->m_properties)
                {
                    if (property.second->m_getter)
                    {
                        RegisterMethod(nodePaletteModel, behaviorContext, categoryPath, behaviorClass, property.first, *property.second->m_getter, ScriptCanvas::PropertyStatus::Getter, behaviorClass->IsMethodOverloaded(property.first));
                    }

                    if (property.second->m_setter)
                    {
                        RegisterMethod(nodePaletteModel, behaviorContext, categoryPath, behaviorClass, property.first, *property.second->m_setter, ScriptCanvas::PropertyStatus::Setter, behaviorClass->IsMethodOverloaded(property.first));
                    }
                }

                for (auto methodIter : behaviorClass->m_methods)
                {
                    if (!IsExplicitOverload(*methodIter.second))
                    {
                        // Respect the exclusion flags
                        auto attributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, methodIter.second->m_attributes));
                        if (ShouldExcludeFromNodeList(attributeData , {}))
                        {
                            continue;
                        }

                        RegisterMethod(nodePaletteModel, behaviorContext, categoryPath, behaviorClass, methodIter.first, *methodIter.second, ScriptCanvas::PropertyStatus::None, behaviorClass->IsMethodOverloaded(methodIter.first));
                    }
                }
            }
        }
    }

    void PopulateBehaviorContextOverloadedMethods(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::BehaviorContext& behaviorContext)
    {
        for (const AZ::ExplicitOverloadInfo& explicitOverload : behaviorContext.m_explicitOverloads)
        {
            RegisterMethod(nodePaletteModel, behaviorContext, explicitOverload.m_categoryPath, nullptr, explicitOverload.m_name, *explicitOverload.m_overloads.begin()->first, ScriptCanvas::PropertyStatus::None, true);
        }
    }

    void PopulateBehaviorContextEBusHandler(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::BehaviorContext& behaviorContext, const AZ::BehaviorEBus& behaviorEbus)
    {
        if (AZ::ScopedBehaviorEBusHandler handler{ behaviorEbus }; handler)
        {
            auto excludeEbusAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(
                AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorEbus.m_attributes));
            if (ShouldExcludeFromNodeList(excludeEbusAttributeData, handler->RTTI_GetType()))
            {
                return;
            }

            const AZ::BehaviorEBusHandler::EventArray& events(handler->GetEvents());
            if (!events.empty())
            {

                GraphCanvas::TranslationKey key;
                key << "EBusHandler" << behaviorEbus.m_name.c_str() << "details";

                GraphCanvas::TranslationRequests::Details details;
                GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

                AZStd::string categoryPath = details.Category.empty() ? GetCategoryPath(behaviorEbus.m_attributes, behaviorContext) : details.Category;

                // Treat the EBusHandler name as a Category key in order to allow multiple buses to be merged into a single Category.
                {
                    if (!categoryPath.empty())
                    {
                        categoryPath.append("/");
                    }
                    else
                    {
                        categoryPath = "Other/";
                    }

                    if (!details.Name.empty())
                    {
                        categoryPath.append(details.Name.c_str());
                    }
                    else
                    {
                        categoryPath.append(behaviorEbus.m_name.c_str());
                    }
                }

                for (const auto& event : events)
                {
                    nodePaletteModel.RegisterEBusHandlerNodeModelInformation(categoryPath.c_str(), behaviorEbus.m_name, event.m_name, ScriptCanvas::EBusBusId(behaviorEbus.m_name), event);
                }
            }
        }
    }

    void PopulateBehaviorContextEBusEventMethods(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::BehaviorContext& behaviorContext, const AZ::BehaviorEBus& behaviorEbus)
    {
        if (!behaviorEbus.m_events.empty())
        {
            GraphCanvas::TranslationKey key;
            key << "EBusSender" << behaviorEbus.m_name.c_str() << "details";

            GraphCanvas::TranslationRequests::Details details;
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            AZStd::string categoryPath = details.Category.empty() ? GetCategoryPath(behaviorEbus.m_attributes, behaviorContext) : details.Category;

            // Parent
            AZStd::string displayName = details.Name;

            // Treat the EBus name as a Category key in order to allow multiple buses to be merged into a single Category.
            if (!categoryPath.empty())
            {
                categoryPath.append("/");
            }
            else
            {
                categoryPath = "Other/";
            }

            if (!details.Name.empty())
            {
                categoryPath.append(details.Name.c_str());
            }
            else
            {
                categoryPath.append(behaviorEbus.m_name.c_str());
            }

            ScriptCanvasEditor::CategoryInformation ebusCategoryInformation;

            ebusCategoryInformation.m_tooltip = details.Tooltip;

            nodePaletteModel.RegisterCategoryInformation(categoryPath, ebusCategoryInformation);

            for (auto event : behaviorEbus.m_events)
            {
                if (IsDeprecated(event.second.m_attributes))
                {
                    continue;
                }

                auto excludeEventAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, event.second.m_attributes));
                if (ShouldExcludeFromNodeList(excludeEventAttributeData, AZ::Uuid::CreateNull()))
                {
                    continue; // skip this event
                }

                const bool isOverload{ false }; // overloaded events are not trivially supported
                nodePaletteModel.RegisterEBusSenderNodeModelInformation(categoryPath, behaviorEbus.m_name, event.first, ScriptCanvas::EBusBusId(behaviorEbus.m_name.c_str()), ScriptCanvas::EBusEventId(event.first.c_str()), event.second, ScriptCanvas::PropertyStatus::None, isOverload);
            }
        }
    }

    void PopulateBehaviorContextEBuses(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel,
        const AZ::BehaviorContext& behaviorContext)
    {
        AZStd::unordered_set<AZ::Crc32> skipBuses = GetEBusExcludeSet(behaviorContext);

        for (const auto& [ebusName, behaviorEbus] : behaviorContext.m_ebuses)
        {
            if (behaviorEbus == nullptr)
            {
                continue;
            }

            auto skipBusIterator = skipBuses.find(AZ::Crc32(ebusName));
            if (skipBusIterator != skipBuses.end())
            {
                continue;
            }

            // Skip buses mapped by their deprecated name (usually duplicates)
            if (ebusName == behaviorEbus->m_deprecatedName)
            {
                continue;
            }

            // Only bind Behavior Buses marked with the Scope type of Launcher
            if (!AZ::Internal::IsInScope(behaviorEbus->m_attributes, AZ::Script::Attributes::ScopeFlags::Launcher))
            {
                continue; // skip this bus
            }

            if (IsDeprecated(behaviorEbus->m_attributes))
            {
                continue;
            }

            auto excludeEbusAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(
                AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorEbus->m_attributes));
            if (ShouldExcludeFromNodeList(excludeEbusAttributeData, AZ::Uuid::CreateNull()))
            {
                continue;
            }

            if (auto runtimeEbusAttributePtr = AZ::FindAttribute(AZ::RuntimeEBusAttribute, behaviorEbus->m_attributes))
            {
                bool isRuntimeEbus = false;
                AZ::AttributeReader(nullptr, runtimeEbusAttributePtr).Read<bool>(isRuntimeEbus);

                if (isRuntimeEbus)
                {
                    continue;
                }
            }

            // EBus Handler
            PopulateBehaviorContextEBusHandler(nodePaletteModel, behaviorContext, *behaviorEbus);

            // EBus Sender
            PopulateBehaviorContextEBusEventMethods(nodePaletteModel, behaviorContext, *behaviorEbus);
        }
    }

    // Helper function for populating the node palette model.
    // Pulled out just to make the tabbing a bit nicer, since it's a huge method.
    void PopulateNodePaletteModel(ScriptCanvasEditor::NodePaletteModel& nodePaletteModel)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

        AZ_Assert(serializeContext, "Could not find SerializeContext. Aborting Palette Creation.");
        AZ_Assert(behaviorContext, "Could not find BehaviorContext. Aborting Palette Creation.");

        if (serializeContext == nullptr || behaviorContext == nullptr)
        {
            return;
        }

        // Populates the NodePalette with each ScriptCanvas LibraryDefinition derived class
        // static InitNodeRegistry() function
        PopulateScriptCanvasDerivedNodes(nodePaletteModel, *serializeContext);

        // Populates the VariablePalette with type registered with the ScriptCanvas DataRegistry
        PopulateVariablePalette();

        // Populates the NodePalette with Behavior Class method nodes 
        PopulateBehaviorContextClassMethods(nodePaletteModel, *behaviorContext);

        // Populates the NodePalette with BehaviorContext methods overloaded on the same name
        PopulateBehaviorContextOverloadedMethods(nodePaletteModel, *behaviorContext);

        // Populates the NodePalette with EBus Event method nodes and EBus Event handler nodes
        PopulateBehaviorContextEBuses(nodePaletteModel, *behaviorContext);

        // Populates the NodePalette with Methods reflected directly on the BehaviorContext
        PopulateBehaviorContextGlobalMethods(nodePaletteModel, *behaviorContext);

        // Populates the NodePalette with Properties reflected directly on the BehaviorContext
        PopulateBehaviorContextGlobalProperties(nodePaletteModel, *behaviorContext);
    }
}

namespace ScriptCanvasEditor
{
    ////////////////////////////////
    // NodePaletteModelInformation
    ////////////////////////////////
    void NodePaletteModelInformation::PopulateTreeItem(GraphCanvas::NodePaletteTreeItem& treeItem) const
    {
        if (!m_toolTip.empty())
        {
            treeItem.SetToolTip(m_toolTip.c_str());
        }

        if (!m_styleOverride.empty())
        {
            treeItem.SetStyleOverride(m_styleOverride.c_str());
        }

        if (!m_titlePaletteOverride.empty())
        {
            const bool forceSet = true;
            treeItem.SetTitlePalette(m_titlePaletteOverride.c_str(), forceSet);
        }
    }

    /////////////////////
    // NodePaletteModel
    /////////////////////

    NodePaletteModel::NodePaletteModel()
        : m_paletteId(AZ::Entity::MakeId())
    {
        UpgradeNotificationsBus::Handler::BusConnect();
    }

    NodePaletteModel::~NodePaletteModel()
    {
        UpgradeNotificationsBus::Handler::BusDisconnect();

        DisconnectLambdas();

        ClearRegistry();
    }

    NodePaletteId NodePaletteModel::GetNotificationId() const
    {
        return m_paletteId;
    }

    void NodePaletteModel::AssignAssetModel(AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
    {
        m_assetModel = assetModel;

        if (m_assetModel)
        {
            TraverseTree();

            ConnectLambdas();
        }
    }

    void NodePaletteModel::ConnectLambdas()
    {
        {
            auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsInserted, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsInserted(parentIndex, first, last); });
            m_lambdaConnections.emplace_back(connection);
        }

        {
            auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsAboutToBeRemoved, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsAboutToBeRemoved(parentIndex, first, last); });
            m_lambdaConnections.emplace_back(connection);
        }
    }

    void NodePaletteModel::DisconnectLambdas()
    {
        for (auto connection : m_lambdaConnections)
        {
            QObject::disconnect(connection);
        }
    }

    void NodePaletteModel::RepopulateModel()
    {
        ClearRegistry();

        PopulateNodePaletteModel((*this));

        if (m_assetModel)
        {
            TraverseTree();
        }

        NodePaletteModelNotificationBus::Event(m_paletteId, &NodePaletteModelNotifications::OnAssetModelRepopulated);
    }

    void NodePaletteModel::RegisterCustomNode(AZStd::string_view categoryPath, const AZ::Uuid& uuid, AZStd::string_view name, const AZ::SerializeContext::ClassData* classData)
    {
        ScriptCanvas::NodeTypeIdentifier nodeIdentifier = ScriptCanvas::NodeUtils::ConstructCustomNodeIdentifier(uuid);

        auto mapIter = m_registeredNodes.find(nodeIdentifier);

        if (mapIter == m_registeredNodes.end())
        {
            CustomNodeModelInformation* customNodeInformation = aznew CustomNodeModelInformation();

            customNodeInformation->m_nodeIdentifier = nodeIdentifier;
            customNodeInformation->m_typeId = uuid;
            customNodeInformation->m_displayName = name;
            customNodeInformation->m_categoryPath = categoryPath;

            bool isDeprecated(false);

            if (classData && classData->m_editData && classData->m_editData->m_name)
            {
                GraphCanvas::TranslationKey key;
                key << "ScriptCanvas::Node" << classData->m_typeId.ToString<AZStd::string>().c_str() << "details";

                GraphCanvas::TranslationRequests::Details details;
                GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

                if (details.Name.empty())
                {
                    details.Name = classData->m_editData->m_name;
                    details.Tooltip = classData->m_editData->m_description;
                    // TODO-LS: here I can dump the missing data into a JSON file to encourage easy fixing
                    // of the missing data
                }

                customNodeInformation->m_displayName = details.Name;
                customNodeInformation->m_toolTip = details.Tooltip;

                if (!details.Category.empty())
                {
                    customNodeInformation->m_categoryPath = details.Category;
                }

                if (customNodeInformation->m_displayName.empty())
                {
                    customNodeInformation->m_displayName = classData->m_editData->m_name;
                }

                auto editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);

                if (editorDataElement)
                {
                    if (auto categoryStyleAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                    {
                        if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryStyleAttribute))
                        {
                            if (categoryAttributeData->Get(nullptr))
                            {
                                customNodeInformation->m_styleOverride = categoryAttributeData->Get(nullptr);
                            }
                        }
                    }

                    if (auto titlePaletteAttribute = editorDataElement->FindAttribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride))
                    {
                        if (auto titlePaletteAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(titlePaletteAttribute))
                        {
                            if (titlePaletteAttributeData->Get(nullptr))
                            {
                                customNodeInformation->m_titlePaletteOverride = titlePaletteAttributeData->Get(nullptr);
                            }
                        }
                    }


                    if (auto deprecatedAttribute = editorDataElement->FindAttribute(AZ::Script::Attributes::Deprecated))
                    {
                        if (auto deprecatedAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<bool>*>(deprecatedAttribute))
                        {
                            isDeprecated = deprecatedAttributeData->Get(nullptr);
                        }
                    }

                    if (customNodeInformation->m_toolTip.empty() && classData->m_editData->m_description)
                    {
                        customNodeInformation->m_toolTip = classData->m_editData->m_description;
                    }
                }
            }

            if (!isDeprecated)
            {
                m_registeredNodes.emplace(AZStd::make_pair(nodeIdentifier, customNodeInformation));
            }
            else
            {
                delete customNodeInformation;
            }
        }
    }

    void NodePaletteModel::RegisterClassNode
        ( const AZStd::string& categoryPath
        , const AZStd::string& methodClass
        , const AZStd::string& methodName
        , const AZ::BehaviorMethod*
        , const AZ::BehaviorContext*
        , ScriptCanvas::PropertyStatus propertyStatus
        , bool isOverload)
    {
        ScriptCanvas::NodeTypeIdentifier nodeIdentifier = isOverload ? ScriptCanvas::NodeUtils::ConstructMethodOverloadedNodeIdentifier(methodName) : ScriptCanvas::NodeUtils::ConstructMethodNodeIdentifier(methodClass, methodName, propertyStatus);

        auto registerIter = m_registeredNodes.find(nodeIdentifier);

        if (registerIter == m_registeredNodes.end())
        {            
            MethodNodeModelInformation* methodModelInformation = aznew MethodNodeModelInformation();
            methodModelInformation->m_isOverload = isOverload;
            methodModelInformation->m_nodeIdentifier = nodeIdentifier;
            methodModelInformation->m_classMethod = methodClass;
            methodModelInformation->m_methodName = methodName;
            methodModelInformation->m_propertyStatus = propertyStatus;
            methodModelInformation->m_titlePaletteOverride = "MethodNodeTitlePalette";

            GraphCanvas::TranslationKey catkey;
            catkey << "BehaviorClass" << methodClass.c_str() << "details";
            GraphCanvas::TranslationRequests::Details catdetails;
            GraphCanvas::TranslationRequestBus::BroadcastResult(catdetails, &GraphCanvas::TranslationRequests::GetDetails, catkey, catdetails);

            GraphCanvas::TranslationKey key;
            key << "BehaviorClass" << methodClass.c_str() << "methods" << methodName.c_str() << "details";

            GraphCanvas::TranslationRequests::Details details;
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            methodModelInformation->m_displayName = details.Name.empty() ? methodName : details.Name;
            methodModelInformation->m_toolTip = details.Tooltip;
            methodModelInformation->m_categoryPath = categoryPath;

            if (methodModelInformation->m_categoryPath.empty())
            {
                methodModelInformation->m_categoryPath = "Other";
            }

            m_registeredNodes.emplace(AZStd::make_pair(nodeIdentifier, methodModelInformation));
        }        
    }

    void NodePaletteModel::RegisterGlobalConstant(const AZ::BehaviorContext&, const AZ::BehaviorProperty* behaviorProperty, const AZ::BehaviorMethod& behaviorMethod)
    {
        // Construct Node Identifier using the BehaviorMethod name and the ScriptCanvas Method typeid
        ScriptCanvas::NodeTypeIdentifier nodeIdentifier =
            ScriptCanvas::NodeUtils::ConstructGlobalMethodNodeIdentifier(behaviorMethod.m_name);

        // Register the methodModelInformation if not already registered
        if (auto registerIter = m_registeredNodes.find(nodeIdentifier); registerIter == m_registeredNodes.end())
        {
            auto  methodModelInformation = AZStd::make_unique<GlobalMethodNodeModelInformation>();
            methodModelInformation->m_methodName = behaviorMethod.m_name;
            methodModelInformation->m_nodeIdentifier = nodeIdentifier;

            methodModelInformation->m_titlePaletteOverride = "MethodNodeTitlePalette";

            GraphCanvas::TranslationKey key;
            key << "Constant" << behaviorProperty->m_name.c_str() << "details";

            GraphCanvas::TranslationRequests::Details details;
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            methodModelInformation->m_displayName = details.Name;
            methodModelInformation->m_toolTip = details.Tooltip;
            methodModelInformation->m_categoryPath = details.Category;

            if (methodModelInformation->m_categoryPath.empty())
            {
                methodModelInformation->m_categoryPath = "Constants";
            }

            m_registeredNodes.emplace(nodeIdentifier, methodModelInformation.release());
        }
    }

    void NodePaletteModel::RegisterMethodNode(const AZ::BehaviorContext&, const AZ::BehaviorMethod& behaviorMethod)
    {
        // Construct Node Identifier using the BehaviorMethod name and the ScriptCanvas Method typeid
        ScriptCanvas::NodeTypeIdentifier nodeIdentifier =
            ScriptCanvas::NodeUtils::ConstructGlobalMethodNodeIdentifier(behaviorMethod.m_name);

        // Register the methodModelInformation if not already registered
        if (auto registerIter = m_registeredNodes.find(nodeIdentifier); registerIter == m_registeredNodes.end())
        {
            auto  methodModelInformation = AZStd::make_unique<GlobalMethodNodeModelInformation>();
            methodModelInformation->m_methodName = behaviorMethod.m_name;
            methodModelInformation->m_nodeIdentifier = nodeIdentifier;
            methodModelInformation->m_titlePaletteOverride = "MethodNodeTitlePalette";

            GraphCanvas::TranslationKey key;
            key << "BehaviorMethod" << behaviorMethod.m_name.c_str() << "details";

            GraphCanvas::TranslationRequests::Details details;
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            methodModelInformation->m_displayName = details.Name.empty() ? behaviorMethod.m_name : details.Name;
            methodModelInformation->m_toolTip = details.Tooltip.empty() ? "" : details.Tooltip;
            methodModelInformation->m_categoryPath = details.Category.empty() ? "Behavior Context: Global Methods" : details.Category;

            m_registeredNodes.emplace(nodeIdentifier, methodModelInformation.release());
        }
    }

    void NodePaletteModel::RegisterEBusHandlerNodeModelInformation(AZStd::string_view categoryPath, AZStd::string_view busName, AZStd::string_view eventName, const ScriptCanvas::EBusBusId& busId, const AZ::BehaviorEBusHandler::BusForwarderEvent& forwardEvent)
    {
        ScriptCanvas::NodeTypeIdentifier nodeIdentifier = ScriptCanvas::NodeUtils::ConstructEBusEventReceiverIdentifier(busId, forwardEvent.m_eventId);

        auto nodeIter = m_registeredNodes.find(nodeIdentifier);

        if (nodeIter == m_registeredNodes.end())
        {
            EBusHandlerNodeModelInformation* handlerInformation = aznew EBusHandlerNodeModelInformation();
            
            handlerInformation->m_titlePaletteOverride = "HandlerNodeTitlePalette";
            handlerInformation->m_categoryPath = categoryPath;
            handlerInformation->m_nodeIdentifier = nodeIdentifier;

            handlerInformation->m_busName = busName;
            handlerInformation->m_eventName = eventName;
            handlerInformation->m_busId = busId;
            handlerInformation->m_eventId = forwardEvent.m_eventId;

            GraphCanvas::TranslationKey key;
            key << "EBusHandler" << busName << "methods" << eventName << "details";

            GraphCanvas::TranslationRequests::Details details;
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            handlerInformation->m_displayName = details.Name.empty() ? eventName : details.Name.c_str();
            handlerInformation->m_toolTip = details.Tooltip.empty() ? "" : details.Tooltip;

            m_registeredNodes.emplace(AZStd::make_pair(nodeIdentifier, handlerInformation));
        }
    }

    void NodePaletteModel::RegisterEBusSenderNodeModelInformation
        ( AZStd::string_view categoryPath
        , AZStd::string_view busName
        , AZStd::string_view eventName
        , const ScriptCanvas::EBusBusId& busId
        , const ScriptCanvas::EBusEventId& eventId
        , const AZ::BehaviorEBusEventSender&
        , ScriptCanvas::PropertyStatus propertyStatus
        , bool isOverload)
    {
        ScriptCanvas::NodeTypeIdentifier nodeIdentifier = isOverload ? ScriptCanvas::NodeUtils::ConstructEBusEventSenderOverloadedIdentifier(busId, eventId) : ScriptCanvas::NodeUtils::ConstructEBusEventSenderIdentifier(busId, eventId);

        auto nodeIter = m_registeredNodes.find(nodeIdentifier);

        if (nodeIter == m_registeredNodes.end())
        {
            EBusSenderNodeModelInformation* senderInformation = aznew EBusSenderNodeModelInformation();

            senderInformation->m_isOverload = isOverload;
            senderInformation->m_titlePaletteOverride = "MethodNodeTitlePalette";
            senderInformation->m_categoryPath = categoryPath;
            senderInformation->m_nodeIdentifier = nodeIdentifier;
            senderInformation->m_propertyStatus = propertyStatus;

            senderInformation->m_busName = busName;
            senderInformation->m_eventName = eventName;
            senderInformation->m_busId = busId;
            senderInformation->m_eventId = eventId;

            GraphCanvas::TranslationKey key;
            key << "EBusSender" << busName << "methods" << eventName << "details";

            GraphCanvas::TranslationRequests::Details details;
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            senderInformation->m_displayName = details.Name.empty() ? eventName : details.Name.c_str();
            senderInformation->m_toolTip = details.Tooltip.empty() ? "" : details.Tooltip;

            m_registeredNodes.emplace(AZStd::make_pair(nodeIdentifier, senderInformation));
        }
    }

    AZStd::vector<ScriptCanvas::NodeTypeIdentifier> NodePaletteModel::RegisterScriptEvent(ScriptEvents::ScriptEventsAsset* scriptEventAsset)
    {
        const ScriptEvents::ScriptEvent& scriptEvent = scriptEventAsset->m_definition;

        ScriptCanvas::EBusBusId busId = scriptEventAsset->GetBusId();

        AZStd::string category = scriptEvent.GetCategory();
        
        auto methods = scriptEvent.GetMethods();

        AZStd::vector<ScriptCanvas::NodeTypeIdentifier> identifiers;

        // Each event has a handler and a receiver
        identifiers.reserve(methods.size() * 2);

        for (const auto& method : methods)
        {
            ScriptCanvas::EBusEventId eventId = method.GetEventId();

            ScriptCanvas::NodeTypeIdentifier senderIdentifier = ScriptCanvas::NodeUtils::ConstructSendScriptEventIdentifier(busId, eventId);
            ScriptCanvas::NodeTypeIdentifier receiverIdentifier = ScriptCanvas::NodeUtils::ConstructScriptEventReceiverIdentifier(busId, eventId);

            ScriptEventHandlerNodeModelInformation* handlerInformation = aznew ScriptEventHandlerNodeModelInformation();

            handlerInformation->m_titlePaletteOverride = "HandlerNodeTitlePalette";
            handlerInformation->m_busName = scriptEvent.GetName();
            handlerInformation->m_eventName = method.GetName();
            handlerInformation->m_displayName = method.GetName();
            handlerInformation->m_categoryPath = scriptEvent.GetCategory();

            handlerInformation->m_busId = busId;
            handlerInformation->m_eventId = eventId;
            handlerInformation->m_nodeIdentifier = receiverIdentifier;

            m_registeredNodes.emplace(AZStd::make_pair(receiverIdentifier, handlerInformation));

            ScriptEventSenderNodeModelInformation* senderInformation = aznew ScriptEventSenderNodeModelInformation();
            
            senderInformation->m_titlePaletteOverride = "MethodNodeTitlePalette";
            senderInformation->m_busName = scriptEvent.GetName();
            senderInformation->m_eventName = method.GetName();
            senderInformation->m_displayName = method.GetName();
            senderInformation->m_categoryPath = scriptEvent.GetCategory();

            senderInformation->m_busId = busId;
            senderInformation->m_eventId = eventId;
            senderInformation->m_nodeIdentifier = senderIdentifier;

            m_registeredNodes.emplace(AZStd::make_pair(senderIdentifier, senderInformation));

            m_assetMapping.insert(AZStd::make_pair(scriptEventAsset->GetId(), senderIdentifier));
            m_assetMapping.insert(AZStd::make_pair(scriptEventAsset->GetId(), receiverIdentifier));

            identifiers.emplace_back(senderIdentifier);
            identifiers.emplace_back(receiverIdentifier);
        }

        return identifiers;
    }

    void NodePaletteModel::RegisterCategoryInformation(const AZStd::string& category, const CategoryInformation& categoryInformation)
    {
        auto categoryIter = m_categoryInformation.find(category);

        if (categoryIter == m_categoryInformation.end())
        {
            m_categoryInformation[category] = categoryInformation;
        }
    }

    const CategoryInformation* NodePaletteModel::FindCategoryInformation(const AZStd::string& categoryStyle) const
    {
        auto categoryIter = m_categoryInformation.find(categoryStyle);

        if (categoryIter != m_categoryInformation.end())
        {
            return &(categoryIter->second);
        }

        return nullptr;
    }

    const CategoryInformation* NodePaletteModel::FindBestCategoryInformation(AZStd::string_view categoryView) const
    {
        const CategoryInformation* bestCategoryFit = nullptr;

        auto categoryIter = m_categoryInformation.find(categoryView);

        size_t offset = AZStd::string_view::npos;

        AZStd::string_view categoryTrail = categoryView;

        while (categoryIter == m_categoryInformation.end() && !categoryTrail.empty())
        {
            size_t seperator = categoryTrail.find_last_of('/', offset);

            if (seperator == AZStd::string_view::npos)
            {
                categoryTrail = {};
            }
            else
            {
                categoryTrail = categoryTrail.substr(0, seperator - 1);
                categoryIter = m_categoryInformation.find(categoryTrail);
            }
        }

        if (categoryIter != m_categoryInformation.end())
        {
            bestCategoryFit = &(categoryIter->second);
        }

        return bestCategoryFit;
    }

    const NodePaletteModelInformation* NodePaletteModel::FindNodePaletteInformation(const ScriptCanvas::NodeTypeIdentifier& nodeType) const
    {
        auto registryIter = m_registeredNodes.find(nodeType);

        if (registryIter != m_registeredNodes.end())
        {
            return registryIter->second;
        }

        return nullptr;
    }

    const NodePaletteModel::NodePaletteRegistry& NodePaletteModel::GetNodeRegistry() const
    {
        return m_registeredNodes;
    }

    GraphCanvas::GraphCanvasTreeItem* NodePaletteModel::CreateCategoryNode(AZStd::string_view categoryPath, AZStd::string_view categoryName, GraphCanvas::GraphCanvasTreeItem* parentItem) const
    {
        GraphCanvas::NodePaletteTreeItem* treeItem = parentItem->CreateChildNode<GraphCanvas::NodePaletteTreeItem>(categoryName, ScriptCanvasEditor::AssetEditorId);

        const CategoryInformation* categoryInformation = FindCategoryInformation(categoryPath);

        if (categoryInformation)
        {
            if (!categoryInformation->m_tooltip.empty())
            {
                treeItem->SetToolTip(categoryInformation->m_tooltip.c_str());
            }

            if (!categoryInformation->m_paletteOverride.empty())
            {
                treeItem->SetTitlePalette(categoryInformation->m_paletteOverride.c_str());
            }

            if (!categoryInformation->m_styleOverride.empty())
            {
                treeItem->SetStyleOverride(categoryInformation->m_styleOverride.c_str());
            }
        }

        return treeItem;
    }

    void NodePaletteModel::OnRowsInserted(const QModelIndex& parentIndex, int first, int last)
    {
        for (int i = first; i <= last; ++i)
        {
            QModelIndex modelIndex = m_assetModel->index(i, 0, parentIndex);
            QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());
            auto nodeTypeIdentifiers = ProcessAsset(entry);

            for (auto nodeTypeIdentifier : nodeTypeIdentifiers)
            {
                auto nodeIter = m_registeredNodes.find(nodeTypeIdentifier);

                if (nodeIter != m_registeredNodes.end())
                {
                    NodePaletteModelNotificationBus::Event(m_paletteId, &NodePaletteModelNotifications::OnAssetNodeAdded, nodeIter->second);
                }
            }
        }
    }

    void NodePaletteModel::OnRowsAboutToBeRemoved(const QModelIndex& parentIndex, int first, int last)
    {
        for (int i = first; i <= last; ++i)
        {
            QModelIndex modelIndex = m_assetModel->index(first, 0, parentIndex);
            QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

            const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
            {
                const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

                if (productEntry)
                {
                    RemoveAsset(productEntry->GetAssetId());
                }
            }
        }
    }

    void NodePaletteModel::TraverseTree(QModelIndex index)
    {
        QModelIndex sourceIndex = m_assetModel->mapToSource(index);
        AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

        ProcessAsset(entry);

        int rowCount = m_assetModel->rowCount(index);

        for (int i = 0; i < rowCount; ++i)
        {
            QModelIndex nextIndex = m_assetModel->index(i, 0, index);
            TraverseTree(nextIndex);
        }
    }

    AZStd::vector<ScriptCanvas::NodeTypeIdentifier> NodePaletteModel::ProcessAsset(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> myLocker(m_mutex);

        if (entry)
        {
            if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
            {
                const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = static_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

                if (productEntry->GetAssetType() == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
                {
                    const AZ::Data::AssetId& assetId = productEntry->GetAssetId();
                    
                    auto busAsset = AZ::Data::AssetManager::Instance().GetAsset(assetId, azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), AZ::Data::AssetLoadBehavior::PreLoad);
                    busAsset.BlockUntilLoadComplete();

                    if (busAsset.IsReady())
                    {
                        ScriptEvents::ScriptEventsAsset* data = busAsset.GetAs<ScriptEvents::ScriptEventsAsset>();

                        return RegisterScriptEvent(data);
                    }
                    else
                    {
                        AZ_TracePrintf("NodePaletteModel", "Could not refresh node palette properly, the asset failed to load correctly.");
                    }
                }
            }
        }

        return AZStd::vector<ScriptCanvas::NodeTypeIdentifier>();
    }

    void NodePaletteModel::RemoveAsset(const AZ::Data::AssetId& assetId)
    {
        auto mapRange = m_assetMapping.equal_range(assetId);

        for (auto rangeIter = mapRange.first; rangeIter != mapRange.second; ++rangeIter)
        {
            auto nodeIter = m_registeredNodes.find(rangeIter->second);

            if (nodeIter != m_registeredNodes.end())
            {
                NodePaletteModelNotificationBus::Event(m_paletteId, &NodePaletteModelNotifications::OnAssetNodeRemoved, nodeIter->second);
                delete nodeIter->second;
                m_registeredNodes.erase(nodeIter);
            }
        }
    }

    void NodePaletteModel::ClearRegistry()
    {
        for (auto& mapPair : m_registeredNodes)
        {
            delete mapPair.second;
        }

        m_registeredNodes.clear();

        m_categoryInformation.clear();
    }
}
