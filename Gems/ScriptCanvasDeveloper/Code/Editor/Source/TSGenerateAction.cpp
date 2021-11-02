/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvasDeveloperEditor/TSGenerateAction.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>

#include <XMLDoc.h>

#if !defined(Q_MOC_RUN)
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>
#endif

#include <Source/Translation/TranslationAsset.h>

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include "../Translation/TranslationHelper.h"

#include <Tools/TranslationBrowser/TranslationBrowser.h>

#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeConfiguration.h>

#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace ScriptCanvasDeveloperEditor
{
    namespace TranslationGenerator
    {
        AZStd::string GetContextName(const AZ::SerializeContext::ClassData& classData)
        {
            if (auto editorDataElement = classData.m_editData ? classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData) : nullptr)
            {
                if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
                {
                    if (auto data = azrtti_cast<AZ::Edit::AttributeData<const char*>*>(attribute))
                    {
                        AZStd::string fullCategoryName = data->Get(nullptr);
                        if (!fullCategoryName.empty())
                        {
                            AZStd::string delimiter = "/";
                            AZStd::vector<AZStd::string> results;
                            AZStd::tokenize(fullCategoryName, delimiter, results);
                            return results.back();
                        }
                    }
                }
            }

            return {};
        }

        AZStd::string GetLibraryCategory(const AZ::SerializeContext& serializeContext, const AZStd::string& nodeName)
        {
            AZStd::string category;

            // Get all the types.
            auto EnumerateLibraryDefintionNodes = [&nodeName, &category , &serializeContext](
                const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
            {

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

                        /*if (auto categoryStyleAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
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
                        }*/
                    }
                }

                // Children
                for (auto& node : ScriptCanvas::Library::LibraryDefinition::GetNodes(classData->m_typeId))
                {
                    //if (HasExcludeFromNodeListAttribute(&serializeContext, node.first))
                    //{
                    //    continue;
                    //}

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
                        if (node.second == nodeName)
                        {
                            category = categoryPath;
                            return false;
                        }
                        //nodePaletteModel.RegisterCustomNode(categoryPath, node.first, node.second, nodeClassData);
                    }
                }

                return true;
            };

            const AZ::TypeId& libraryDefTypeId = azrtti_typeid<ScriptCanvas::Library::LibraryDefinition>();
            serializeContext.EnumerateDerived(EnumerateLibraryDefintionNodes, libraryDefTypeId, libraryDefTypeId);

            return category;
        }

        AZStd::string GetCategory(const AZ::SerializeContext::ClassData* classData)
        {
            AZStd::string categoryPath;

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
                }
            }

            return categoryPath;
        }

        void OpenTranslationBrowser(QWidget* parent = nullptr)
        {
            using namespace ScriptCanvasDeveloper;
            TranslationBrowser* browser = new TranslationBrowser(parent);
            browser->show();
        }

        void ReloadTranslation(QWidget*)
        {
            GraphCanvas::TranslationRequestBus::Broadcast(&GraphCanvas::TranslationRequests::Restore);
            ScriptCanvasEditor::AssetTrackerRequestBus::Broadcast(&ScriptCanvasEditor::AssetTrackerRequests::RefreshAll);

            

        }

        QAction* TranslationDatabaseFileAction(QMenu* mainMenu, QWidget* mainWindow)
        {
            QAction* qAction = nullptr;

            if (mainWindow)
            {
                qAction = new QAction(QAction::tr("Produce Localization Files for All Types"), mainWindow);
                qAction->setAutoRepeat(false);
                qAction->setToolTip("Produces a .names file for every reflected type supported by scripting.");
                qAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Alt+X", "Debug|Produce Localization Database")));
                mainWindow->addAction(qAction);
                mainWindow->connect(qAction, &QAction::triggered, &GenerateTranslationDatabase);

                qAction = mainMenu->addAction(QAction::tr("Translation Browser"));

                qAction->setAutoRepeat(false);
                qAction->setToolTip("....");
                qAction->setShortcut(QAction::tr("Ctrl+Alt+T", "Developer|Translation Browser"));
                QObject::connect(qAction, &QAction::triggered, [mainWindow]() {OpenTranslationBrowser(mainWindow); });

                qAction = mainMenu->addAction(QAction::tr("Reload Translation"));
                qAction->setAutoRepeat(false);
                qAction->setToolTip("....");
                qAction->setShortcut(QAction::tr("Ctrl+Alt+R", "Developer|Reload Translation"));
                QObject::connect(qAction, &QAction::triggered, [mainWindow]() {ReloadTranslation(mainWindow); });

            }

            return qAction;
        }

        template <typename T>
        bool ShouldSkip(const T* object)
        {
            using namespace AZ::Script::Attributes;

            // Check for "ignore" attribute for ScriptCanvas
            auto excludeClassAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<ExcludeFlags>*>(AZ::FindAttribute(ExcludeFrom, object->m_attributes));
            const bool excludeClass = excludeClassAttributeData && (static_cast<AZ::u64>(excludeClassAttributeData->Get(nullptr)) & static_cast<AZ::u64>(ExcludeFlags::List | ExcludeFlags::Documentation));

            if (excludeClass)
            {
                return true; // skip this class
            }

            return false;
        }

        void WriteString(rapidjson::Value& owner, const AZStd::string& key, const AZStd::string& value, rapidjson::Document& document)
        {
            if (key.empty() || value.empty())
            {
                return;
            }

            rapidjson::Value item(rapidjson::kStringType);
            item.SetString(value.c_str(), document.GetAllocator());

            rapidjson::Value keyVal(rapidjson::kStringType);
            keyVal.SetString(key.c_str(), document.GetAllocator());

            owner.AddMember(keyVal, item, document.GetAllocator());
        }

        void GetTypeNameAndDescription(AZ::TypeId typeId, AZStd::string& outName, AZStd::string& outDescription)
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Serialize Context is required");

            if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId))
            {
                if (classData->m_editData)
                {
                    outName = classData->m_editData->m_name ? classData->m_editData->m_name : classData->m_name;
                    outDescription = classData->m_editData->m_description ? classData->m_editData->m_description : "";
                }
                else
                {
                    outName = classData->m_name;
                }
            }
        }


        AZStd::list<AZ::BehaviorEBus*> GatherCandidateEBuses(AZ::SerializeContext* /*serializeContext*/, AZ::BehaviorContext* behaviorContext)
        {
            AZStd::list<AZ::BehaviorEBus*> candidates;

            // We will skip buses that are ONLY registered on classes that derive from EditorComponentBase,
            // because they don't have a runtime implementation. Buses such as the TransformComponent which
            // is implemented by both an EditorComponentBase derived class and a Component derived class
            // will still appear
            AZStd::unordered_set<AZ::Crc32> skipBuses;
            AZStd::unordered_set<AZ::Crc32> potentialSkipBuses;
            AZStd::unordered_set<AZ::Crc32> nonSkipBuses;

            for (const auto& classIter : behaviorContext->m_classes)
            {
                const AZ::BehaviorClass* behaviorClass = classIter.second;

                if (ShouldSkip(behaviorClass))
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

            for (const auto& ebusIter : behaviorContext->m_ebuses)
            {
                [[maybe_unused]] bool addContext = false;
                AZ::BehaviorEBus* ebus = ebusIter.second;

                if (ebus == nullptr)
                {
                    continue;
                }

                auto excludeEbusAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, ebusIter.second->m_attributes));
                const bool excludeBus = excludeEbusAttributeData && static_cast<AZ::u64>(excludeEbusAttributeData->Get(nullptr))& static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);

                auto skipBusIterator = skipBuses.find(AZ::Crc32(ebusIter.first.c_str()));
                if (!ebus || skipBusIterator != skipBuses.end() || excludeBus)
                {
                    continue;
                }

                candidates.push_back(ebus);
            }

            return candidates;
        }

        bool TranslatedEBusHandler([[maybe_unused]] AZ::BehaviorContext* behaviorContext, AZ::BehaviorEBus* ebus, TranslationFormat& translationRoot)
        {
            if (!ebus)
            {
                return false;
            }

            if (!ebus->m_createHandler || !ebus->m_destroyHandler)
            {
                return false;
            }

            AZ::BehaviorEBusHandler* handler(nullptr);
            if (ebus->m_createHandler->InvokeResult(handler))
            {
                Entry entry;

                // Generate the translation file
                entry.m_key = ebus->m_name;
                entry.m_context = "EBusHandler";

                // Check if the Handler has a translation
                AZStd::string translationContextHandler = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name);

                AZStd::string translationHandlerKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Name);
                AZStd::string translationHandlerTooltipKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Tooltip);
                AZStd::string translationHandlerCategoryKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Category);

                GraphCanvas::TranslationKeyedString translatedHandlerName(ebus->m_name, translationContextHandler, translationHandlerKey);
                GraphCanvas::TranslationKeyedString translatedHandlerTooltip(ebus->m_toolTip, translationContextHandler, translationHandlerTooltipKey);
                GraphCanvas::TranslationKeyedString translatedHandlerCateogry(GraphCanvasAttributeHelper::GetStringAttribute(ebus, AZ::Script::Attributes::Category), translationContextHandler, translationHandlerCategoryKey);

                entry.m_details.m_name = translatedHandlerName.GetDisplayString();
                entry.m_details.m_tooltip = translatedHandlerTooltip.GetDisplayString();
                entry.m_details.m_category = translatedHandlerCateogry.GetDisplayString();

                AZStd::string tempEbusName = ebus->m_name;
                AZStd::to_upper(tempEbusName.begin(), tempEbusName.end());


                for (const AZ::BehaviorEBusHandler::BusForwarderEvent& event : handler->GetEvents())
                {
                    Method methodEntry;

                    AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(event.m_name);
                    methodEntry.m_key = cleanName;
                    methodEntry.m_details.m_category = "";
                    methodEntry.m_details.m_tooltip = "";
                    methodEntry.m_details.m_name = event.m_name;

                    AZStd::string translatedName = ScriptCanvasEditor::TranslationHelper::GetKeyTranslation(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name, event.m_name, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Name);
                    AZStd::string translatedTooltip = ScriptCanvasEditor::TranslationHelper::GetKeyTranslation(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name, event.m_name, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Tooltip);


                    AZStd::string oldEventName = event.m_name;
                    AZStd::to_upper(oldEventName.begin(), oldEventName.end());
                    AZStd::string oldEventKey = AZStd::string::format("%s_NAME", oldEventName.c_str());
                    AZStd::string oldEventTooltipKey = AZStd::string::format("%s_TOOLTIP", oldEventName.c_str());

                    GraphCanvas::TranslationKeyedString translatedEventName(cleanName, translationContextHandler, oldEventKey);
                    GraphCanvas::TranslationKeyedString translatedEventTooltip("", translationContextHandler, oldEventTooltipKey);

                    methodEntry.m_details.m_name = !translatedName.empty() ? translatedName.c_str() : translatedEventName.GetDisplayString();
                    methodEntry.m_details.m_tooltip = !translatedTooltip.empty() ? translatedTooltip.c_str() : translatedEventTooltip.GetDisplayString();

                    // Arguments (Input Slots)
                    if (!event.m_parameters.empty())
                    {
                        for (size_t argIndex = AZ::eBehaviorBusForwarderEventIndices::ParameterFirst; argIndex < event.m_parameters.size(); ++argIndex)
                        {
                            const AZ::BehaviorParameter& parameter = event.m_parameters[argIndex];

                            Argument argument;

                            AZStd::string argumentKey = parameter.m_typeId.ToString<AZStd::string>();
                            AZStd::string argumentName = event.m_name;
                            AZStd::string argumentDescription = "";

                            // Generate the OLD style translation key
                            AZStd::string oldKey = AZStd::string::format("HANDLER_%s_%s_OUTPUT%zu_NAME", tempEbusName.c_str(), oldEventName.c_str(), argIndex - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst);
                            AZStd::string oldTooltipKey = AZStd::string::format("HANDLER_%s_%s_OUTPUT%zu_TOOLTIP", tempEbusName.c_str(), oldEventName.c_str(), argIndex - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst);

                            if (!event.m_metadataParameters.empty() && event.m_metadataParameters.size() > argIndex)
                            {
                                argumentName = event.m_metadataParameters[argIndex].m_name;
                                argumentDescription = event.m_metadataParameters[argIndex].m_toolTip;
                            }

                            if (argumentName.empty())
                            {
                                GetTypeNameAndDescription(parameter.m_typeId, argumentName, argumentDescription);
                            }

                            GraphCanvas::TranslationKeyedString translatedArgName(argumentName, translationContextHandler, oldKey);
                            GraphCanvas::TranslationKeyedString translatedArgTooltip(argumentDescription, translationContextHandler, oldTooltipKey);

                            argument.m_typeId = argumentKey;
                            argument.m_details.m_name = translatedArgName.GetDisplayString();
                            argument.m_details.m_tooltip = translatedArgTooltip.GetDisplayString();

                            methodEntry.m_arguments.push_back(argument);
                        }
                    }

                    auto resultIndex = AZ::eBehaviorBusForwarderEventIndices::Result;
                    const AZ::BehaviorParameter* resultParameter = event.HasResult() ? &event.m_parameters[resultIndex] : nullptr;
                    if (resultParameter)
                    {
                        Argument result;

                        AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

                        AZStd::string resultName = event.m_name;
                        AZStd::string resultDescription = "";

                        if (!event.m_metadataParameters.empty() && event.m_metadataParameters.size() > resultIndex)
                        {
                            resultName = event.m_metadataParameters[resultIndex].m_name;
                            resultDescription = event.m_metadataParameters[resultIndex].m_toolTip;
                        }

                        if (resultName.empty())
                        {
                            GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);
                        }

                        AZStd::string oldKey = AZStd::string::format("HANDLER_%s_%s_OUTPUT%d_NAME", tempEbusName.c_str(), oldEventName.c_str(), 0);
                        AZStd::string oldTooltipKey = AZStd::string::format("HANDLER_%s_%s_OUTPUT%d_TOOLTIP", tempEbusName.c_str(), oldEventName.c_str(), 0);

                        GraphCanvas::TranslationKeyedString oldReturnName(resultName, translationContextHandler, oldKey);
                        GraphCanvas::TranslationKeyedString oldReturnTooltip(resultDescription, translationContextHandler, oldTooltipKey);

                        result.m_typeId = resultKey;
                        result.m_details.m_name = oldReturnName.GetDisplayString();
                        result.m_details.m_tooltip = oldReturnTooltip.GetDisplayString();

                        methodEntry.m_results.push_back(result);
                    }

                    entry.m_methods.push_back(methodEntry);

                }

                ebus->m_destroyHandler->Invoke(handler); // Destroys the Created EbusHandler

                translationRoot.m_entries.push_back(entry);
            }

            if (!translationRoot.m_entries.empty())
            {
                return true;
            }

            return false;

        }

        void SaveJSONData(const AZStd::string& filename, TranslationFormat& translationRoot)
        {
            rapidjson::Document document;
            document.SetObject();
            rapidjson::Value entries(rapidjson::kArrayType);

            // Here I'll need to parse translationRoot myself and produce the JSON
            for (const auto& entrySource : translationRoot.m_entries)
            {
                rapidjson::Value entry(rapidjson::kObjectType);
                rapidjson::Value value(rapidjson::kStringType);

                value.SetString(entrySource.m_key.c_str(), document.GetAllocator());
                entry.AddMember("key", value, document.GetAllocator());

                value.SetString(entrySource.m_context.c_str(), document.GetAllocator());
                entry.AddMember("context", value, document.GetAllocator());

                value.SetString(entrySource.m_variant.c_str(), document.GetAllocator());
                entry.AddMember("variant", value, document.GetAllocator());

                rapidjson::Value details(rapidjson::kObjectType);
                value.SetString(entrySource.m_details.m_name.c_str(), document.GetAllocator());
                details.AddMember("name", value, document.GetAllocator());

                WriteString(details, "category", entrySource.m_details.m_category, document);
                WriteString(details, "tooltip", entrySource.m_details.m_tooltip, document);
                WriteString(details, "subtitle", entrySource.m_details.m_subtitle, document);

                entry.AddMember("details", details, document.GetAllocator());

                if (!entrySource.m_methods.empty())
                {
                    rapidjson::Value methods(rapidjson::kArrayType);

                    for (const auto& methodSource : entrySource.m_methods)
                    {
                        rapidjson::Value theMethod(rapidjson::kObjectType);

                        value.SetString(methodSource.m_key.c_str(), document.GetAllocator());
                        theMethod.AddMember("key", value, document.GetAllocator());

                        if (!methodSource.m_context.empty())
                        {
                            value.SetString(methodSource.m_context.c_str(), document.GetAllocator());
                            theMethod.AddMember("context", value, document.GetAllocator());
                        }

                        if (!methodSource.m_entry.m_name.empty())
                        {
                            rapidjson::Value entrySlot(rapidjson::kObjectType);
                            value.SetString(methodSource.m_entry.m_name.c_str(), document.GetAllocator());
                            entrySlot.AddMember("name", value, document.GetAllocator());

                            WriteString(entrySlot, "tooltip", methodSource.m_entry.m_tooltip, document);

                            theMethod.AddMember("entry", entrySlot, document.GetAllocator());
                        }

                        if (!methodSource.m_exit.m_name.empty())
                        {
                            rapidjson::Value exitSlot(rapidjson::kObjectType);
                            value.SetString(methodSource.m_exit.m_name.c_str(), document.GetAllocator());
                            exitSlot.AddMember("name", value, document.GetAllocator());

                            WriteString(exitSlot, "tooltip", methodSource.m_exit.m_tooltip, document);

                            theMethod.AddMember("exit", exitSlot, document.GetAllocator());
                        }

                        rapidjson::Value methodDetails(rapidjson::kObjectType);

                        value.SetString(methodSource.m_details.m_name.c_str(), document.GetAllocator());
                        methodDetails.AddMember("name", value, document.GetAllocator());

                        WriteString(methodDetails, "category", methodSource.m_details.m_category, document);
                        WriteString(methodDetails, "tooltip", methodSource.m_details.m_tooltip, document);

                        theMethod.AddMember("details", methodDetails, document.GetAllocator());

                        if (!methodSource.m_arguments.empty())
                        {
                            rapidjson::Value methodArguments(rapidjson::kArrayType);

                            [[maybe_unused]] size_t index = 0;
                            for (const auto& argSource : methodSource.m_arguments)
                            {
                                rapidjson::Value argument(rapidjson::kObjectType);
                                rapidjson::Value argumentDetails(rapidjson::kObjectType);

                                value.SetString(argSource.m_typeId.c_str(), document.GetAllocator());
                                argument.AddMember("typeid", value, document.GetAllocator());

                                value.SetString(argSource.m_details.m_name.c_str(), document.GetAllocator());
                                argumentDetails.AddMember("name", value, document.GetAllocator());

                                WriteString(argumentDetails, "category", argSource.m_details.m_category, document);
                                WriteString(argumentDetails, "tooltip", argSource.m_details.m_tooltip, document);


                                argument.AddMember("details", argumentDetails, document.GetAllocator());

                                methodArguments.PushBack(argument, document.GetAllocator());

                            }

                            theMethod.AddMember("params", methodArguments, document.GetAllocator());

                        }

                        if (!methodSource.m_results.empty())
                        {
                            rapidjson::Value methodArguments(rapidjson::kArrayType);

                            for (const auto& argSource : methodSource.m_results)
                            {
                                rapidjson::Value argument(rapidjson::kObjectType);
                                rapidjson::Value argumentDetails(rapidjson::kObjectType);

                                value.SetString(argSource.m_typeId.c_str(), document.GetAllocator());
                                argument.AddMember("typeid", value, document.GetAllocator());

                                value.SetString(argSource.m_details.m_name.c_str(), document.GetAllocator());
                                argumentDetails.AddMember("name", value, document.GetAllocator());

                                WriteString(argumentDetails, "category", argSource.m_details.m_category, document);
                                WriteString(argumentDetails, "tooltip", argSource.m_details.m_tooltip, document);

                                argument.AddMember("details", argumentDetails, document.GetAllocator());

                                methodArguments.PushBack(argument, document.GetAllocator());
                            }

                            
                            theMethod.AddMember("results", methodArguments, document.GetAllocator());

                        }

                        methods.PushBack(theMethod, document.GetAllocator());
                    }

                    entry.AddMember("methods", methods, document.GetAllocator());
                }

                if (!entrySource.m_slots.empty())
                {
                    rapidjson::Value slotsArray(rapidjson::kArrayType);

                    for (const auto& slotSource : entrySource.m_slots)
                    {
                        rapidjson::Value theSlot(rapidjson::kObjectType);

                        value.SetString(slotSource.m_key.c_str(), document.GetAllocator());
                        theSlot.AddMember("key", value, document.GetAllocator());

                        rapidjson::Value sloDetails(rapidjson::kObjectType);
                        if (!slotSource.m_details.m_name.empty())
                        {
                            WriteString(sloDetails, "name", slotSource.m_details.m_name, document);
                            WriteString(sloDetails, "tooltip", slotSource.m_details.m_tooltip, document);
                            theSlot.AddMember("details", sloDetails, document.GetAllocator());
                        }

                        if (!slotSource.m_data.m_details.m_name.empty())
                        {
                            rapidjson::Value slotDataDetails(rapidjson::kObjectType);
                            WriteString(slotDataDetails, "name", slotSource.m_data.m_details.m_name, document);
                            theSlot.AddMember("details", slotDataDetails, document.GetAllocator());
                        }

                        slotsArray.PushBack(theSlot, document.GetAllocator());
                    }

                    entry.AddMember("slots", slotsArray, document.GetAllocator());
                }

                entries.PushBack(entry, document.GetAllocator());
            }

            document.AddMember("entries", entries, document.GetAllocator());

            AZStd::string translationOutputFolder = AZStd::string::format("@engroot@/TranslationAssets");
            AZStd::string outputFileName = AZStd::string::format("%s/%s.names", translationOutputFolder.c_str(), filename.c_str());

            AZStd::string endPath;
            AZ::StringFunc::Path::GetFolderPath(outputFileName.c_str(), endPath);

            if (!AZ::IO::FileIOBase::GetInstance()->Exists(endPath.c_str()))
            {
                if (AZ::IO::FileIOBase::GetInstance()->CreatePath(endPath.c_str()) != AZ::IO::ResultCode::Success)
                {
                    AZ_Error("Translation", false, "Failed to create output folder");
                    return;
                }
            }

            char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(outputFileName.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);
            endPath = resolvedBuffer;
            AZ::StringFunc::Path::Normalize(endPath);

            AZ::IO::SystemFile outputFile;
            if (!outputFile.Open(endPath.c_str(),
                AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE |
                AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
                AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
            {
                AZ_Error("Translation", false, "Failed to open file for writing: %s", filename.c_str());
                return;
            }

            rapidjson::StringBuffer scratchBuffer;

            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(scratchBuffer);
            document.Accept(writer);

            outputFile.Write(scratchBuffer.GetString(), scratchBuffer.GetSize());
            outputFile.Close();

            scratchBuffer.Clear();
        }

        AZ::Entity* TranslateAZEvent(const AZ::BehaviorMethod& method)
        {
            // Make sure the method returns an AZ::Event by reference or pointer
            if (AZ::MethodReturnsAzEventByReferenceOrPointer(method))
            {
                // Read in AZ Event Description data to retrieve the event name and parameter names
                AZ::Attribute* azEventDescAttribute = AZ::FindAttribute(AZ::Script::Attributes::AzEventDescription, method.m_attributes);
                AZ::BehaviorAzEventDescription behaviorAzEventDesc;
                AZ::AttributeReader azEventDescAttributeReader(nullptr, azEventDescAttribute);
                azEventDescAttributeReader.Read<decltype(behaviorAzEventDesc)>(behaviorAzEventDesc);
                if (behaviorAzEventDesc.m_eventName.empty())
                {
                    AZ_Error("NodeUtils", false, "Cannot create an AzEvent node with empty event name")
                }

                auto scriptCanvasEntity = aznew AZ::Entity{ AZStd::string::format("SC-EventNode(%s)", behaviorAzEventDesc.m_eventName.c_str()) };
                scriptCanvasEntity->Init();
                auto azEventHandler = scriptCanvasEntity->CreateComponent<ScriptCanvas::Nodes::Core::AzEventHandler>();
                
                azEventHandler->InitEventFromMethod(method);

                return scriptCanvasEntity;
            }

            return nullptr;
        }

        void TranslateAZEvents([[maybe_unused]] AZ::SerializeContext* serializeContext, AZ::BehaviorContext* behaviorContext)
        {
            (void)behaviorContext;
            GraphCanvas::TranslationKey translationKey;
            AZStd::vector<AZ::Entity*> nodes;

            // Methods
            for (const auto& behaviorMethod : behaviorContext->m_methods)
            {
                const auto& method = *behaviorMethod.second;
                AZ::Entity* node = TranslateAZEvent(method);
                if (node)
                {
                    nodes.push_back(node);
                }
            }

            // Methods in classes
            for (auto behaviorClass : behaviorContext->m_classes)
            {
                for (auto behaviorMethod : behaviorClass.second->m_methods)
                {
                    const auto& method = *behaviorMethod.second;
                    AZ::Entity* node = TranslateAZEvent(method);
                    if (node)
                    {
                        nodes.push_back(node);
                    }
                }
            }

            TranslationFormat translationRoot;

            for (auto& node : nodes)
            {
                ScriptCanvas::Nodes::Core::AzEventHandler* nodeComponent = node->FindComponent<ScriptCanvas::Nodes::Core::AzEventHandler>();
                nodeComponent->Init();
                nodeComponent->Configure();

                const ScriptCanvas::Nodes::Core::AzEventEntry& azEventEntry{ nodeComponent->GetEventEntry() };

                Entry entry;
                entry.m_key = azEventEntry.m_eventName;
                entry.m_context = "AZEventHandler";
                entry.m_details.m_name = azEventEntry.m_eventName;

                for (const ScriptCanvas::Slot& slot : nodeComponent->GetSlots())
                {
                    Slot slotEntry;

                    GraphCanvas::SlotGroup group = GraphCanvas::SlotGroups::Invalid;

                    if (slot.IsVisible())
                    {
                        slotEntry.m_key = slot.GetName();

                        if (slot.GetId() == azEventEntry.m_azEventInputSlotId)
                        {
                            GraphCanvas::TranslationKeyedString slotTranslationEntry(azEventEntry.m_eventName);
                            slotTranslationEntry.m_context = ScriptCanvasEditor::TranslationHelper::GetAzEventHandlerContextKey();
                            // The translation key in this case acts like a json pointer referencing a particular
                            // json string within a hypothetical json document
                            AZ::StackedString azEventHandlerNodeKey = ScriptCanvasEditor::TranslationHelper::GetAzEventHandlerRootPointer(azEventEntry.m_eventName);

                            azEventHandlerNodeKey.Push("Name");
                            slotTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };
                            
                            azEventHandlerNodeKey.Pop();
                            azEventHandlerNodeKey.Push("Tooltip");
                            slotTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };

                            slotEntry.m_details.m_name = slotTranslationEntry.GetDisplayString();
                        }
                        else
                        {
                            GraphCanvas::TranslationKeyedString slotTranslationEntry(slot.GetName());
                            slotTranslationEntry.m_context = ScriptCanvasEditor::TranslationHelper::GetAzEventHandlerContextKey();
                            // The translation key in this case acts like a json pointer referencing a particular
                            // json string within a hypothetical json document
                            // translation key is rooted at /AzEventHandler/${EventName}/Slots/${SlotName}/{In,Out,Param,Return}
                            AZ::StackedString azEventHandlerNodeKey = ScriptCanvasEditor::TranslationHelper::GetAzEventHandlerRootPointer(azEventEntry.m_eventName);
                            azEventHandlerNodeKey.Push("Slots");
                            azEventHandlerNodeKey.Push(slot.GetName());
                            switch (ScriptCanvasEditor::TranslationHelper::GetItemType(slot.GetDescriptor()))
                            {
                            case ScriptCanvasEditor::TranslationItemType::ExecutionInSlot:
                                azEventHandlerNodeKey.Push("In");
                                break;
                            case ScriptCanvasEditor::TranslationItemType::ExecutionOutSlot:
                                azEventHandlerNodeKey.Push("Out");
                                break;
                            case ScriptCanvasEditor::TranslationItemType::ParamDataSlot:
                                azEventHandlerNodeKey.Push("Param");
                                break;
                            case ScriptCanvasEditor::TranslationItemType::ReturnDataSlot:
                                azEventHandlerNodeKey.Push("Return");
                                break;
                            default:
                                // Slot is not an execution or data slot, do nothing
                                break;
                            }

                            slotEntry.m_details.m_name = slotTranslationEntry.GetDisplayString();

                        }

                        entry.m_slots.push_back(slotEntry);
                    }
                }

                translationRoot.m_entries.push_back(entry);

                // delete the node, don't need to keep it beyond this point
                delete node;


                AZStd::string filename = GraphCanvas::TranslationKey::Sanitize(entry.m_key);

                AZStd::string targetFile = AZStd::string::format("AZEvents/%s", filename.c_str());

                SaveJSONData(targetFile, translationRoot);

                translationRoot.m_entries.clear();
            }


        }

        void TranslateNodes(AZ::SerializeContext* serializeContext, TranslationFormat& translationRoot)
        {
            GraphCanvas::TranslationKey translationKey;
            AZStd::vector<AZ::TypeId> nodes;

            auto getNodeClasses = [&serializeContext, &nodes](const AZ::SerializeContext::ClassData*, const AZ::Uuid& type)
            {

                bool foundBaseClass = false;
                auto baseClassVisitorFn = [&nodes, &type, &foundBaseClass](const AZ::SerializeContext::ClassData* reflectedBase, const AZ::TypeId& /*rttiBase*/)
                {
                    if (!reflectedBase)
                    {
                        foundBaseClass = false;
                        return false; // stop iterating
                    }

                    foundBaseClass = (reflectedBase->m_typeId == azrtti_typeid<ScriptCanvas::Node>());
                    if (foundBaseClass)
                    {
                        nodes.push_back(type);
                        return false; // we have a base, stop iterating
                    }

                    return true; // keep iterating
                };

                AZ::EntityUtils::EnumerateBaseRecursive(serializeContext, baseClassVisitorFn, type);

                return true;
            };

            serializeContext->EnumerateAll(getNodeClasses);

            for (auto& node : nodes)
            {
                if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(node))
                {
                    Entry entry;
                    entry.m_key = classData->m_typeId.ToString<AZStd::string>();
                    entry.m_context = "ScriptCanvas::Node";

                    EntryDetails& details = entry.m_details;

                    AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(classData->m_name);

                    if (classData->m_editData)
                    {
                        details.m_name = classData->m_editData->m_name;
                    }
                    else
                    {
                        details.m_name = cleanName;
                    }

                    // Tooltip attribute takes priority over the edit data description
                    AZStd::string tooltip = GraphCanvasAttributeHelper::GetStringAttribute(classData, AZ::Script::Attributes::ToolTip);
                    if (!tooltip.empty())
                    {
                        details.m_tooltip = tooltip;
                    }
                    else
                    {
                        details.m_tooltip = classData->m_editData ? classData->m_editData->m_description : "";
                    }

                    // Find the category
                    AZStd::string translationContext;
                    AZStd::string subtitleFallback;
                    auto nodeContext = GetContextName(*classData);
                    GraphCanvas::TranslationKeyedString subtitleKeyedString(nodeContext, translationContext);
                    subtitleKeyedString.m_key = ScriptCanvasEditor::TranslationHelper::GetUserDefinedNodeKey(nodeContext, subtitleFallback, ScriptCanvasEditor::TranslationKeyId::Category);

                    GraphCanvas::TranslationKeyedString categoryKeyedString(GetCategory(classData), nodeContext);
                    categoryKeyedString.m_key = ScriptCanvasEditor::TranslationHelper::GetKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, nodeContext, classData->m_editData->m_name, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);
                    details.m_category = categoryKeyedString.GetDisplayString();


                    details.m_subtitle = subtitleKeyedString.GetDisplayString();
                    if (details.m_subtitle.empty())
                    {
                        details.m_subtitle = details.m_category;
                    }

                    if (details.m_category.empty())
                    {
                        details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(classData, AZ::Script::Attributes::Category);
                        if (details.m_category.empty() && classData->m_editData)
                        {
                            details.m_category = GetCategory(classData);

                            if (details.m_category.empty())
                            {
                                auto elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                                const AZStd::string categoryAttribute = GraphCanvasAttributeHelper::ReadStringAttribute(elementData->m_attributes, AZ::Script::Attributes::Category);
                                if (!categoryAttribute.empty())
                                {
                                    details.m_category = categoryAttribute;
                                }
                            }
                        }
                    }

                    if (details.m_category.empty())
                    {
                        // Get the library's name as the category
                        details.m_category = GetLibraryCategory(*serializeContext, classData->m_name);
                    }

                    if (ScriptCanvas::Node* nodeComponent = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name)))
                    {
                        nodeComponent->Init();
                        nodeComponent->Configure();

                        int inputIndex = 0;
                        int outputIndex = 0;

                        const auto& allSlots = nodeComponent->GetAllSlots();
                        for (const auto& slot : allSlots)
                        {
                            Slot slotEntry;

                            if (slot->GetDescriptor().IsExecution())
                            {
                                if (slot->GetDescriptor().IsInput())
                                {
                                    slotEntry.m_key = AZStd::string::format("Input_%s", slot->GetName().c_str());
                                    inputIndex++;

                                    slotEntry.m_details.m_name = slot->GetName();
                                    slotEntry.m_details.m_tooltip = slot->GetToolTip();
                                }
                                else if (slot->GetDescriptor().IsOutput())
                                {
                                    slotEntry.m_key = AZStd::string::format("Output_%s", slot->GetName().c_str());
                                    outputIndex++;

                                    slotEntry.m_details.m_name = slot->GetName();
                                    slotEntry.m_details.m_tooltip = slot->GetToolTip();
                                }

                                entry.m_slots.push_back(slotEntry);
                            }
                            else
                            {
                                AZStd::string slotTypeKey = slot->GetDataType().IsValid() ? ScriptCanvas::Data::GetName(slot->GetDataType()) : "";
                                if (slotTypeKey.empty())
                                {
                                    if (!slot->GetDataType().GetAZType().IsNull())
                                    {
                                        slotTypeKey = slot->GetDataType().GetAZType().ToString<AZStd::string>();
                                    }
                                }

                                if (slotTypeKey.empty())
                                {
                                    if (slot->GetDynamicDataType() == ScriptCanvas::DynamicDataType::Container)
                                    {
                                        slotTypeKey = "Container";
                                    }
                                    else if (slot->GetDynamicDataType() == ScriptCanvas::DynamicDataType::Value)
                                    {
                                        slotTypeKey = "Value";
                                    }
                                    else if (slot->GetDynamicDataType() == ScriptCanvas::DynamicDataType::Any)
                                    {
                                        slotTypeKey = "Any";
                                    }
                                }

                                Argument& argument = slotEntry.m_data;

                                if (slot->GetDescriptor().IsInput())
                                {
                                    slotEntry.m_key = AZStd::string::format("DataInput_%s", slot->GetName().c_str());
                                    inputIndex++;

                                    AZStd::string argumentKey = slotTypeKey;
                                    AZStd::string argumentName = slot->GetName();
                                    AZStd::string argumentDescription = slot->GetToolTip();

                                    argument.m_typeId = argumentKey;
                                    argument.m_details.m_name = argumentName;
                                    argument.m_details.m_tooltip = argumentDescription;

                                }
                                else if (slot->GetDescriptor().IsOutput())
                                {
                                    slotEntry.m_key = AZStd::string::format("DataOutput_%s", slot->GetName().c_str());
                                    outputIndex++;

                                    AZStd::string resultKey = slotTypeKey;
                                    AZStd::string resultName = slot->GetName();
                                    AZStd::string resultDescription = slot->GetToolTip();

                                    argument.m_typeId = resultKey;
                                    argument.m_details.m_name = resultName;
                                    argument.m_details.m_tooltip = resultDescription;
                                }

                                entry.m_slots.push_back(slotEntry);
                            }
                        }

                        delete nodeComponent;
                    }

                    translationRoot.m_entries.push_back(entry);

                    if (details.m_category.empty())
                    {
                        details.m_category = "Uncategorized";
                    }

                    AZStd::string prefix = GraphCanvas::TranslationKey::Sanitize(details.m_category);
                    AZStd::string filename = GraphCanvas::TranslationKey::Sanitize(details.m_name);

                    AZStd::string targetFile = AZStd::string::format("Nodes/%s_%s", prefix.c_str(), filename.c_str());

                    SaveJSONData(targetFile, translationRoot);

                    translationRoot.m_entries.clear();

                }
            }
        }

        void TranslateOnDemandReflectedTypes([[maybe_unused]] AZ::SerializeContext* serializeContext, AZ::BehaviorContext* behaviorContext, TranslationFormat& translationRoot)
        {
            AZStd::vector<AZ::Uuid> onDemandReflectedTypes;

            for (auto& typePair : behaviorContext->m_typeToClassMap)
            {
                if (behaviorContext->IsOnDemandTypeReflected(typePair.first))
                {
                    onDemandReflectedTypes.push_back(typePair.first);
                }

                // Check for methods that come from node generics
                if (typePair.second->HasAttribute(AZ::ScriptCanvasAttributes::Internal::ImplementedAsNodeGeneric))
                {
                    onDemandReflectedTypes.push_back(typePair.first);
                }
            }

            // Now that I know all the on demand reflected, I'll dump it out
            for (auto& onDemandReflectedType : onDemandReflectedTypes)
            {
                AZ::BehaviorClass* behaviorClass = behaviorContext->m_typeToClassMap[onDemandReflectedType];
                if (behaviorClass)
                {
                    Entry entry;

                    EntryDetails& details = entry.m_details;
                    details.m_name = behaviorClass->m_name;

                    // Get the pretty name
                    AZStd::string prettyName;
                    if (AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes))
                    {
                        AZ::AttributeReader(nullptr, prettyNameAttribute).Read<AZStd::string>(prettyName, *behaviorContext);
                    }

                    entry.m_context = "OnDemandReflected";
                    entry.m_key = behaviorClass->m_typeId.ToString<AZStd::string>().c_str();

                    if (!prettyName.empty())
                    {
                        details.m_name = prettyName;
                    }

                    details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
                    details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::ToolTip);

                    for (auto& methodPair : behaviorClass->m_methods)
                    {
                        AZ::BehaviorMethod* behaviorMethod = methodPair.second;
                        if (behaviorMethod)
                        {
                            Method methodEntry;

                            AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodPair.first);

                            methodEntry.m_key = cleanName;
                            methodEntry.m_context = entry.m_key;

                            methodEntry.m_details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(behaviorMethod, AZ::Script::Attributes::ToolTip);
                            methodEntry.m_details.m_name = methodPair.second->m_name;

                            // Strip the className from the methodName
                            AZStd::string qualifiedName = behaviorClass->m_name + "::";
                            AzFramework::StringFunc::Replace(methodEntry.m_details.m_name, qualifiedName.c_str(), "");

                            AZStd::string cleanMethodName = methodEntry.m_details.m_name;

                            methodEntry.m_entry.m_name = "In";
                            methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", methodEntry.m_details.m_name.c_str());
                            methodEntry.m_exit.m_name = "Out";
                            methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", methodEntry.m_details.m_name.c_str());

                            // Arguments (Input Slots)
                            if (behaviorMethod->GetNumArguments() > 0)
                            {
                                for (size_t argIndex = 0; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
                                {
                                    const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

                                    Argument argument;

                                    AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                                    AZStd::string argumentName = parameter->m_name;
                                    AZStd::string argumentDescription = "";

                                    GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                                    argument.m_typeId = argumentKey;
                                    argument.m_details.m_name = argumentName;
                                    argument.m_details.m_category = "";
                                    argument.m_details.m_tooltip = argumentDescription;

                                    methodEntry.m_arguments.push_back(argument);
                                }
                            }

                            const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
                            if (resultParameter)
                            {
                                Argument result;

                                AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

                                AZStd::string resultName = resultParameter->m_name;
                                AZStd::string resultDescription = "";

                                GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

                                result.m_typeId = resultKey;
                                result.m_details.m_name = resultName;
                                result.m_details.m_tooltip = resultDescription;

                                methodEntry.m_results.push_back(result);
                            }

                            entry.m_methods.push_back(methodEntry);
                        }
                    }

                    translationRoot.m_entries.push_back(entry);
                }

            }
        }

        void TranslateEBus(AZ::SerializeContext* serializeContext, AZ::BehaviorContext* behaviorContext)
        {
            AZStd::list<AZ::BehaviorEBus*> ebuses = GatherCandidateEBuses(serializeContext, behaviorContext);

            // Get the request buses
            for (auto ebus : ebuses)
            {
                if (ShouldSkip(ebus))
                {
                    continue;
                }

                TranslationFormat translationRoot;

                // Get the handlers
                if (!TranslatedEBusHandler(behaviorContext, ebus, translationRoot))
                {
                    if (ebus->m_events.empty())
                    {
                        // Skip empty ebuses
                        continue;
                    }

                    Entry entry;

                     // Generate the translation file
                    entry.m_key = ebus->m_name;
                    entry.m_details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(ebus, AZ::Script::Attributes::Category);;
                    entry.m_details.m_tooltip = ebus->m_toolTip;
                    entry.m_details.m_name = ebus->m_name;
                    entry.m_context = "EBusSender";

                    const AZStd::string prettyName = GraphCanvasAttributeHelper::GetStringAttribute(ebus, AZ::ScriptCanvasAttributes::PrettyName);
                    if (!prettyName.empty())
                    {
                        entry.m_details.m_name = prettyName;
                    }

                    AZStd::string translationContext = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::EbusSender, ebus->m_name);

                    AZStd::string translationSenderKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusSender, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Name);
                    AZStd::string translatedSenderName = QCoreApplication::translate(translationContext.c_str(), translationSenderKey.c_str()).toUtf8().data();

                    if (entry.m_details.m_name.empty() && translationSenderKey != translatedSenderName)
                    {
                        entry.m_details.m_name = translatedSenderName;
                    }

                    AZStd::string translationSenderCategoryKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusSender, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Category);
                    AZStd::string translatedSenderCategory = QCoreApplication::translate(translationContext.c_str(), translationSenderCategoryKey.c_str()).toUtf8().data();
                    if (entry.m_details.m_category.empty() && translationSenderCategoryKey != translatedSenderCategory)
                    {
                        entry.m_details.m_category = translatedSenderCategory;
                    }

                    AZStd::string tempBusName = ebus->m_name;
                    AZStd::to_upper(tempBusName.begin(), tempBusName.end());
                    for (auto event : ebus->m_events)
                    {
                        const AZ::BehaviorEBusEventSender& ebusSender = event.second;

                        AZ::BehaviorMethod* method = ebusSender.m_event;
                        if (!method)
                        {
                            method = ebusSender.m_broadcast;
                        }

                        if (!method)
                        {
                            AZ_Warning("Script Canvas", false, "Failed to find method: %s", event.first.c_str());
                            continue;
                        }

                        Method eventEntry;
                        const char* eventName = event.first.c_str();

                        AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(eventName);
                        eventEntry.m_key = cleanName;

                        AZStd::string oldEventName = eventName;
                        AZStd::to_upper(oldEventName.begin(), oldEventName.end());

                        AZStd::string oldKey = AZStd::string::format("%s_%s_NAME", tempBusName.c_str(), oldEventName.c_str());
                        GraphCanvas::TranslationKeyedString translatedEventName(cleanName, translationContext, oldKey);

                        AZStd::string oldTooltipKey = AZStd::string::format("%s_%s_TOOLTIP", tempBusName.c_str(), oldEventName.c_str());
                        GraphCanvas::TranslationKeyedString translatedEventTooltip(GraphCanvasAttributeHelper::GetStringAttribute(&ebusSender, AZ::Script::Attributes::ToolTip), translationContext, oldTooltipKey);

                        eventEntry.m_details.m_name = translatedEventName.GetDisplayString();
                        eventEntry.m_details.m_tooltip = translatedEventTooltip.GetDisplayString();

                        eventEntry.m_entry.m_name = "In";
                        eventEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", eventEntry.m_details.m_name.c_str());
                        eventEntry.m_exit.m_name = "Out";
                        eventEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", eventEntry.m_details.m_name.c_str());

                        size_t start = method->HasBusId() ? 1 : 0;
                        for (size_t i = start; i < method->GetNumArguments(); ++i)
                        {
                            Argument argument;
                            auto argumentType = method->GetArgument(i)->m_typeId;

                            // Generate the OLD style translation key
                            AZStd::string oldMethodName = cleanName;
                            AZStd::to_upper(oldMethodName.begin(), oldMethodName.end());
                            AZStd::string oldEbusKey = AZStd::string::format("%s_%s_PARAM%zu_NAME", tempBusName.c_str(), oldEventName.c_str(), i - start);
                            AZStd::string oldEBusTooltipKey = AZStd::string::format("%s_%s_PARAM%zu_TOOLTIP", tempBusName.c_str(), oldEventName.c_str(), i - start);

                            GetTypeNameAndDescription(argumentType, argument.m_details.m_name, argument.m_details.m_tooltip);

                            GraphCanvas::TranslationKeyedString oldArgName(argument.m_details.m_name, translationContext, oldEbusKey);
                            GraphCanvas::TranslationKeyedString oldArgTooltip(argument.m_details.m_tooltip, translationContext, oldEBusTooltipKey);

                            argument.m_typeId = argumentType.ToString<AZStd::string>();
                            argument.m_details.m_tooltip = oldArgTooltip.GetDisplayString();
                            argument.m_details.m_name = oldArgName.GetDisplayString();


                            eventEntry.m_arguments.push_back(argument);
                        }

                        if (method->HasResult())
                        {
                            Argument result;

                            AZStd::string oldMethodName = cleanName;
                            AZStd::to_upper(oldMethodName.begin(), oldMethodName.end());
                            AZStd::string oldMethodKey = AZStd::string::format("%s_%s_RESULT%d_NAME", tempBusName.c_str(), oldEventName.c_str(), 0);
                            AZStd::string oldMethodTooltipKey = AZStd::string::format("%s_%s_PARAM%d_TOOLTIP", tempBusName.c_str(), oldEventName.c_str(), 0);

                            auto resultType = method->GetResult()->m_typeId;
                            GetTypeNameAndDescription(resultType, result.m_details.m_name, result.m_details.m_tooltip);

                            GraphCanvas::TranslationKeyedString oldReturnName(result.m_details.m_name, translationContext, oldMethodKey);
                            GraphCanvas::TranslationKeyedString oldReturnTooltip(result.m_details.m_tooltip, translationContext, oldMethodTooltipKey);

                            result.m_typeId = resultType.ToString<AZStd::string>();
                            result.m_details.m_name = oldReturnName.GetDisplayString();
                            result.m_details.m_tooltip = oldReturnTooltip.GetDisplayString();

                            eventEntry.m_results.push_back(result);
                        }

                        entry.m_methods.push_back(eventEntry);

                    }

                    translationRoot.m_entries.push_back(entry);

                    SaveJSONData(AZStd::string::format("EBus/Senders/%s", ebus->m_name.c_str()), translationRoot);
                }
                else
                {
                    SaveJSONData(AZStd::string::format("EBus/Handlers/%s", ebus->m_name.c_str()), translationRoot);
                }
            }
        }

        bool MethodHasAttribute(const AZ::BehaviorMethod* method, AZ::Crc32 attribute)
        {
            return AZ::FindAttribute(attribute, method->m_attributes) != nullptr; // warning C4800: 'AZ::Attribute *': forcing value to bool 'true' or 'false' (performance warning)
        }

        void TranslateBehaviorGlobals(AZ::SerializeContext* /*serializeContext*/, AZ::BehaviorContext* behaviorContext)
        {
            for (const auto& [propertyName, behaviorProperty] : behaviorContext->m_properties)
            {
                TranslationFormat translationRoot;

                if (behaviorProperty->m_getter && !behaviorProperty->m_setter)
                {
                    Entry entry;
                    entry.m_context = "Constant";
                    entry.m_key = propertyName;

                    auto methodName = behaviorProperty->m_getter->m_name;
                    entry.m_details.m_name = methodName;
                    entry.m_details.m_tooltip = behaviorProperty->m_getter->m_debugDescription ? behaviorProperty->m_getter->m_debugDescription : "";

                    auto displayName = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Name);
                    auto toolTip = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Tooltip);
                    auto category= ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);

                    if (!displayName.empty())
                    {
                        entry.m_details.m_name = displayName;
                    }

                    if (!toolTip.empty())
                    {
                        entry.m_details.m_tooltip = toolTip;
                    }

                    entry.m_details.m_category = "Constants";

                    translationRoot.m_entries.push_back(entry);
                }
                else
                {
                    Entry entry;
                    entry.m_context = "BehaviorMethod";

                    if (behaviorProperty->m_getter)
                    {
                        entry.m_key = propertyName;

                        auto methodName = behaviorProperty->m_getter->m_name;
                        entry.m_details.m_name = methodName;
                        entry.m_details.m_tooltip = behaviorProperty->m_getter->m_debugDescription ? behaviorProperty->m_getter->m_debugDescription : "";

                        auto displayName = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Name);
                        auto toolTip = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Tooltip);
                        auto category = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);

                        if (!displayName.empty())
                        {
                            entry.m_details.m_name = displayName;
                        }

                        if (!toolTip.empty())
                        {
                            entry.m_details.m_tooltip = toolTip;
                        }

                        if (!category.empty())
                        {
                            entry.m_details.m_category = category;
                        }

                        translationRoot.m_entries.push_back(entry);
                    }

                    if (behaviorProperty->m_setter)
                    {
                        entry.m_key = propertyName;

                        auto methodName = behaviorProperty->m_setter->m_name;
                        entry.m_details.m_name = methodName;
                        entry.m_details.m_tooltip = behaviorProperty->m_setter->m_debugDescription ? behaviorProperty->m_getter->m_debugDescription : "";

                        auto displayName = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Name);
                        auto toolTip = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Tooltip);
                        auto category = ScriptCanvasEditor::TranslationHelper::GetGlobalMethodKeyTranslation(methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);

                        if (!displayName.empty())
                        {
                            entry.m_details.m_name = displayName;
                        }

                        if (!toolTip.empty())
                        {
                            entry.m_details.m_tooltip = toolTip;
                        }

                        if (!category.empty())
                        {
                            entry.m_details.m_category = category;
                        }

                        translationRoot.m_entries.push_back(entry);
                    }
                }

                AZStd::string fileName = AZStd::string::format("Properties/%s", behaviorProperty->m_name.c_str());
                SaveJSONData(fileName, translationRoot);
            }
        }

        void TranslateMethod(Entry& entry, const char* context, const char* methodName, const AZ::BehaviorMethod& behaviorMethod)
        {
            const char* className = "Global";
            EntryDetails details;
            Method methodEntry;

            AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodName);

            methodEntry.m_key = cleanName;
            methodEntry.m_context = context;

            methodEntry.m_details.m_category = "";
            methodEntry.m_details.m_tooltip = "";
            methodEntry.m_details.m_name = methodName;

            methodEntry.m_entry.m_name = "In";
            methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", cleanName.c_str());
            methodEntry.m_exit.m_name = "Out";
            methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", cleanName.c_str());

            GraphCanvas::TranslationKeyedString methodCategoryString;
            methodCategoryString.m_context = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::GlobalMethod, methodName);
            methodCategoryString.m_key = ScriptCanvasEditor::TranslationHelper::GetKey(ScriptCanvasEditor::TranslationContextGroup::GlobalMethod, context, methodName, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);

            if (!methodCategoryString.GetDisplayString().empty())
            {
                methodEntry.m_details.m_category = methodCategoryString.GetDisplayString();
            }
            else
            {
                if (!MethodHasAttribute(&behaviorMethod, AZ::ScriptCanvasAttributes::FloatingFunction))
                {
                    methodEntry.m_details.m_category = details.m_category;
                }
                else if (MethodHasAttribute(&behaviorMethod, AZ::Script::Attributes::Category))
                {
                    methodEntry.m_details.m_category = GraphCanvasAttributeHelper::ReadStringAttribute(behaviorMethod.m_attributes, AZ::Script::Attributes::Category);;
                }

                if (methodEntry.m_details.m_category.empty())
                {
                    methodEntry.m_details.m_category = "Other";
                }
            }

            AZStd::string translationContext = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::GlobalMethod, methodName);

            // Arguments (Input Slots)
            if (behaviorMethod.GetNumArguments() > 0)
            {
                for (size_t argIndex = 0; argIndex < behaviorMethod.GetNumArguments(); ++argIndex)
                {
                    // Generate the OLD style translation key
                    AZStd::string oldClassName = className;
                    AZStd::to_upper(oldClassName.begin(), oldClassName.end());
                    AZStd::string oldMethodName = cleanName;
                    AZStd::to_upper(oldMethodName.begin(), oldMethodName.end());
                    AZStd::string oldKey = AZStd::string::format("%s_%s_PARAM%zu_NAME", oldClassName.c_str(), oldMethodName.c_str(), argIndex);
                    AZStd::string oldTooltipKey = AZStd::string::format("%s_%s_PARAM%zu_TOOLTIP", oldClassName.c_str(), oldMethodName.c_str(), argIndex);

                    const AZ::BehaviorParameter* parameter = behaviorMethod.GetArgument(argIndex);

                    Argument argument;

                    AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                    AZStd::string argumentName = parameter->m_name;
                    AZStd::string argumentDescription = "";

                    GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                    const AZStd::string parameterName = parameter->m_name;
                    GraphCanvas::TranslationKeyedString oldArgName(parameterName, translationContext, oldKey);

                    GraphCanvas::TranslationKeyedString oldArgTooltip(argumentDescription, translationContext, oldTooltipKey);

                    argument.m_typeId = argumentKey;
                    argument.m_details.m_name = oldArgName.GetDisplayString();
                    argument.m_details.m_category = "";
                    argument.m_details.m_tooltip = oldArgTooltip.GetDisplayString();

                    methodEntry.m_arguments.push_back(argument);
                }
            }

            const AZ::BehaviorParameter* resultParameter = behaviorMethod.HasResult() ? behaviorMethod.GetResult() : nullptr;
            if (resultParameter)
            {
                // Generate the OLD style translation key
                AZStd::string oldClassName = className;
                AZStd::to_upper(oldClassName.begin(), oldClassName.end());
                AZStd::string oldMethodName = cleanName;
                AZStd::to_upper(oldMethodName.begin(), oldMethodName.end());
                AZStd::string oldKey = AZStd::string::format("%s_%s_OUTPUT%d_NAME", oldClassName.c_str(), oldMethodName.c_str(), 0);
                AZStd::string oldTooltipKey = AZStd::string::format("%s_%s_OUTPUT%d_TOOLTIP", oldClassName.c_str(), oldMethodName.c_str(), 0);

                Argument result;

                AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();
                AZStd::string resultName = resultParameter->m_name;
                AZStd::string resultDescription = "";

                GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

                const AZStd::string parameterName = resultParameter->m_name;
                GraphCanvas::TranslationKeyedString oldArgName(parameterName, translationContext, oldKey);
                GraphCanvas::TranslationKeyedString oldArgTooltip(resultDescription, translationContext, oldTooltipKey);

                result.m_typeId = resultKey;
                result.m_details.m_name = oldArgName.GetDisplayString();
                result.m_details.m_tooltip = oldArgTooltip.GetDisplayString();

                methodEntry.m_results.push_back(result);
            }

            entry.m_methods.push_back(methodEntry);
        }

        void TranslateGlobalMethods(AZ::BehaviorContext* behaviorContext)
        {
            for (const auto behaviorMethod : behaviorContext->m_methods)
            {
                TranslationFormat translationRoot;

                Entry entry;
                entry.m_context = "Method";


                entry.m_details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(behaviorMethod.second, AZ::Script::Attributes::Category);
                if (entry.m_details.m_category.empty())
                {
                    entry.m_details.m_category = "Globals"; // TODO: Get the category from bC
                }

                entry.m_key = behaviorMethod.first.c_str();
                TranslateMethod(entry, "Global", behaviorMethod.first.c_str(), *behaviorMethod.second);
                translationRoot.m_entries.push_back(entry);

                AZStd::string fileName = AZStd::string::format("GlobalMethods/%s", behaviorMethod.first.c_str());
                SaveJSONData(fileName, translationRoot);

                translationRoot.m_entries.clear();
            }

        }

        void TranslateBehaviorClasses(AZ::SerializeContext* /*serializeContext*/, AZ::BehaviorContext* behaviorContext)
        {
            for (const auto& classIter : behaviorContext->m_classes)
            {
                const AZ::BehaviorClass* behaviorClass = classIter.second;

                if (ShouldSkip(behaviorClass))  
                {
                    continue;
                }

                AZStd::string className = behaviorClass->m_name;
                AZStd::string prettyName = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::ScriptCanvasAttributes::PrettyName);
                if (!prettyName.empty())
                {
                    className = prettyName;
                }

                {
                    TranslationFormat translationRoot;

                    Entry entry;
                    entry.m_context = "BehaviorClass";
                    entry.m_key = behaviorClass->m_name;

                    EntryDetails& details = entry.m_details;
                    details.m_name = className;
                    details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
                    details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::ToolTip);

                    // Old system data pull
                    AZStd::string translationContext = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, behaviorClass->m_name);
                    AZStd::string translationKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, behaviorClass->m_name, ScriptCanvasEditor::TranslationKeyId::Category);
                    AZStd::string translatedCategory = QCoreApplication::translate(translationContext.c_str(), translationKey.c_str()).toUtf8().data();

                    if (translatedCategory != translationKey)
                    {
                        details.m_category = translatedCategory;
                    }

                    auto translatedName = ScriptCanvasEditor::TranslationHelper::GetClassKeyTranslation(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, classIter.first, ScriptCanvasEditor::TranslationKeyId::Name);
                    if (!translatedName.empty())
                    {
                        details.m_name = translatedName;
                    }

                    if (!behaviorClass->m_methods.empty())
                    {
                        for (const auto& methodPair : behaviorClass->m_methods)
                        {
                            const AZ::BehaviorMethod* behaviorMethod = methodPair.second;

                            Method methodEntry;

                            AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodPair.first);

                            methodEntry.m_key = cleanName;
                            methodEntry.m_context = className;

                            methodEntry.m_details.m_category = "";
                            methodEntry.m_details.m_tooltip = "";
                            methodEntry.m_details.m_name = methodPair.second->m_name;

                            methodEntry.m_entry.m_name = "In";
                            methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", cleanName.c_str());
                            methodEntry.m_exit.m_name = "Out";
                            methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", cleanName.c_str());

                            GraphCanvas::TranslationKeyedString methodCategoryString;
                            methodCategoryString.m_context = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, className.c_str());
                            methodCategoryString.m_key = ScriptCanvasEditor::TranslationHelper::GetKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, className.c_str(), methodPair.first.c_str(), ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);

                            if (!methodCategoryString.GetDisplayString().empty())
                            {
                                methodEntry.m_details.m_category = methodCategoryString.GetDisplayString();
                            }
                            else
                            {
                                if (!MethodHasAttribute(behaviorMethod, AZ::ScriptCanvasAttributes::FloatingFunction))
                                {
                                    methodEntry.m_details.m_category = details.m_category;
                                }
                                else if (MethodHasAttribute(behaviorMethod, AZ::Script::Attributes::Category))
                                {
                                    methodEntry.m_details.m_category = GraphCanvasAttributeHelper::ReadStringAttribute(behaviorMethod->m_attributes, AZ::Script::Attributes::Category);;
                                }

                                if (methodEntry.m_details.m_category.empty())
                                {
                                    methodEntry.m_details.m_category = "Other";
                                }
                            }

                            // Arguments (Input Slots)
                            if (behaviorMethod->GetNumArguments() > 0)
                            {
                                for (size_t argIndex = 0; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
                                {
                                    // Generate the OLD style translation key
                                    AZStd::string oldClassName = className;
                                    AZStd::to_upper(oldClassName.begin(), oldClassName.end());
                                    AZStd::string oldMethodName = cleanName;
                                    AZStd::to_upper(oldMethodName.begin(), oldMethodName.end());
                                    AZStd::string oldKey = AZStd::string::format("%s_%s_PARAM%zu_NAME", oldClassName.c_str(), oldMethodName.c_str(), argIndex);
                                    AZStd::string oldTooltipKey = AZStd::string::format("%s_%s_PARAM%zu_TOOLTIP", oldClassName.c_str(), oldMethodName.c_str(), argIndex);

                                    const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

                                    Argument argument;

                                    AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                                    AZStd::string argumentName = parameter->m_name;
                                    AZStd::string argumentDescription = "";

                                    GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                                    const AZStd::string parameterName = parameter->m_name;
                                    GraphCanvas::TranslationKeyedString oldArgName(parameterName, translationContext, oldKey);

                                    GraphCanvas::TranslationKeyedString oldArgTooltip(argumentDescription, translationContext, oldTooltipKey);

                                    argument.m_typeId = argumentKey;
                                    argument.m_details.m_name = oldArgName.GetDisplayString();
                                    argument.m_details.m_category = "";
                                    argument.m_details.m_tooltip = oldArgTooltip.GetDisplayString();

                                    methodEntry.m_arguments.push_back(argument);
                                }
                            }

                            const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
                            if (resultParameter)
                            {
                                // Generate the OLD style translation key
                                AZStd::string oldClassName = className;
                                AZStd::to_upper(oldClassName.begin(), oldClassName.end());
                                AZStd::string oldMethodName = cleanName;
                                AZStd::to_upper(oldMethodName.begin(), oldMethodName.end());
                                AZStd::string oldKey = AZStd::string::format("%s_%s_OUTPUT%d_NAME", oldClassName.c_str(), oldMethodName.c_str(), 0);
                                AZStd::string oldTooltipKey = AZStd::string::format("%s_%s_OUTPUT%d_TOOLTIP", oldClassName.c_str(), oldMethodName.c_str(), 0);

                                Argument result;

                                AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();
                                AZStd::string resultName = resultParameter->m_name;
                                AZStd::string resultDescription = "";

                                GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

                                const AZStd::string parameterName = resultParameter->m_name;
                                GraphCanvas::TranslationKeyedString oldArgName(parameterName, translationContext, oldKey);
                                GraphCanvas::TranslationKeyedString oldArgTooltip(resultDescription, translationContext, oldTooltipKey);

                                result.m_typeId = resultKey;
                                result.m_details.m_name = oldArgName.GetDisplayString();
                                result.m_details.m_tooltip = oldArgTooltip.GetDisplayString();

                                methodEntry.m_results.push_back(result);
                            }

                            entry.m_methods.push_back(methodEntry);
                        }
                    }

                    translationRoot.m_entries.push_back(entry);

                    AZStd::string fileName = AZStd::string::format("Classes/%s", className.c_str());

                    SaveJSONData(fileName, translationRoot);
                }
            }
        }

        void GenerateTranslationDatabase()
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            AZ::BehaviorContext* behaviorContext{};
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

            AZ_Assert(serializeContext && behaviorContext, "Must have valid Serialization and Behavior Contexts");

            // Global Methods
            {
                TranslateGlobalMethods(behaviorContext);
            }

            // BehaviorClass
            {
                TranslateBehaviorClasses(serializeContext, behaviorContext);
            }

            // On Demand Reflected Types
            {
                TranslationFormat onDemandTranslationRoot;
                TranslateOnDemandReflectedTypes(serializeContext, behaviorContext, onDemandTranslationRoot);
                SaveJSONData("Types/OnDemandReflectedTypes", onDemandTranslationRoot);
            }

            // Native nodes
            {
                TranslationFormat nodeTranslationRoot;
                TranslateNodes(serializeContext, nodeTranslationRoot);
            }

            // EBus
            {
                TranslateEBus(serializeContext, behaviorContext);
            }

            // Globals
            {
                TranslateBehaviorGlobals(serializeContext, behaviorContext);
            }

            // AZ::Events
            {
                TranslateAZEvents(serializeContext, behaviorContext);
            }


            char buffer[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@engroot@/TranslationAssets", buffer, sizeof(buffer));
            AZ_TracePrintf("Script Canvas", AZStd::string::format("Translation Database Generation Complete, see: %s", buffer).c_str());
        }
    } // TranslationGenerator
} // ScriptCanvasDeveloperEditor
