/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string_view.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvasDeveloperEditor/NodeListDumpAction.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>

namespace ScriptCanvasDeveloperEditor
{
    namespace NodeListDumpAction
    {
        void DumpBehaviorContextNodes();
        void DumpBehaviorContextMethods(AZStd::string& dumpStr);
        void DumpBehaviorContextEbuses(AZStd::string& dumpStr);

        QAction* CreateNodeListDumpAction(QMenu* mainMenu)
        {
            QAction* nodeDumpAction = nullptr;

            if (mainMenu)
            {
                nodeDumpAction = mainMenu->addAction(QAction::tr("Dump EBus Nodes"));
                nodeDumpAction->setAutoRepeat(false);
                nodeDumpAction->setToolTip("Dumps a list of all EBus nodes(their inputs and outputs) to the clipboard");
                nodeDumpAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Alt+N", "Debug|Dump EBus Nodes")));

                QObject::connect(nodeDumpAction, &QAction::triggered, &DumpBehaviorContextNodes);
            }

            return nodeDumpAction;
        }

        namespace DumpInternal
        {
            const size_t maxInputParameters = 10;
        }
        void DumpBehaviorContextNodes()
        {
            AZStd::string nodeList = "Group Name,Class Name/Ebus Name,Event Name/Method Name,Output Type";

            for (size_t i = 0; i < DumpInternal::maxInputParameters; ++i)
            {
                nodeList += ",Input Type " + AZStd::to_string(aznumeric_cast<int>(i) + 1);
            }
            nodeList += "\n";

            DumpBehaviorContextMethods(nodeList);
            DumpBehaviorContextEbuses(nodeList);

            QMimeData* mime = new QMimeData;
            mime->setData("text/plain", QByteArray(nodeList.cbegin(), static_cast<int>(nodeList.size())));
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setMimeData(mime);
        }

        void DumpBehaviorContextMethods(AZStd::string& dumpStr)
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            AZ::BehaviorContext* behaviorContext{};
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

            if (serializeContext == nullptr || behaviorContext == nullptr)
            {
                return;
            }

            for (const auto& classIter : behaviorContext->m_classes)
            {
                const AZ::BehaviorClass* behaviorClass = classIter.second;

                // Check for "ignore" attribute for ScriptCanvas
                auto excludeClassAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));
                const bool excludeClass = excludeClassAttributeData && static_cast<AZ::u64>(excludeClassAttributeData->Get(nullptr)) & static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);
                if (excludeClass)
                {
                    continue; // skip this class
                }

                AZStd::string_view categoryName;
                if (auto categoryAttribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::Category, behaviorClass->m_attributes)))
                {
                    categoryName = categoryAttribute->Get(nullptr);
                }

                for (auto methodPair : behaviorClass->m_methods)
                {
                    // Check for "ignore" attribute for ScriptCanvas
                    auto excludeMethodAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, methodPair.second->m_attributes));
                    const bool excludeMethod = excludeMethodAttributeData && static_cast<AZ::u64>(excludeMethodAttributeData->Get(nullptr)) & static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);
                    if (excludeMethod)
                    {
                        continue; // skip this method
                    }

                    dumpStr += AZStd::string::format(R"("%s","%s","%s",)", !categoryName.empty() ? categoryName.data() : "", classIter.first.c_str(), methodPair.first.c_str());
                    AZ::BehaviorMethod* method = methodPair.second;
                    const auto result = method->HasResult() ? method->GetResult() : nullptr;
                    if (result)
                    {
                        AZStd::string resultType = result->m_name;
                        dumpStr += "\"" + resultType + "\"";
                    }
                    else
                    {
                        dumpStr += "\"void\"";
                    }

                    for (size_t i = 0; i < method->GetNumArguments(); ++i)
                    {
                        dumpStr += ",";
                        if (const AZ::BehaviorParameter* argument = method->GetArgument(i))
                        {
                            AZStd::string argType = argument->m_name;
                            dumpStr += "\"" + argType;
                            const AZStd::string* argName = method->GetArgumentName(i);
                            if (argName && !argName->empty())
                            {
                                dumpStr += "(" + *argName + ")";
                                const AZStd::string* argToolTip = method->GetArgumentToolTip(i);
                                if (argToolTip && !argToolTip->empty())
                                {
                                    dumpStr += ": " + *argToolTip;
                                }
                            }
                            else if (i == 0 && method->IsMember())
                            {
                                dumpStr += "(This Pointer)";
                            }

                            dumpStr += "\"";
                        }
                    }

                    for (size_t i = method->GetNumArguments(); i < DumpInternal::maxInputParameters; ++i)
                    {
                        dumpStr += ",";
                    }
                    dumpStr += "\n";
                }
            }
        }

        void DumpBehaviorContextEBusHandlers(AZStd::string& dumpStr, AZ::BehaviorEBus* ebus, AZStd::string_view categoryName)
        {
            if (!ebus)
            {
                return;
            }

            if (!ebus->m_createHandler || !ebus->m_destroyHandler)
            {
                return;
            }

            AZ::BehaviorContext* behaviorContext{};
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

            AZ::BehaviorEBusHandler* handler;
            if (ebus->m_createHandler->InvokeResult(handler))
            {
                for (const AZ::BehaviorEBusHandler::BusForwarderEvent& event : handler->GetEvents())
                {
                    dumpStr += AZStd::string::format(R"("%s","%s","%s",)", !categoryName.empty() ? categoryName.data() : "", ebus->m_name.c_str(), event.m_name);
                    if (event.HasResult())
                    {
                        const AZ::BehaviorParameter& resultParam = event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result];
                        AZStd::string resultType = resultParam.m_name;
                        dumpStr += "\"" + resultType + "\"";
                    }
                    else
                    {
                        dumpStr += "\"void\"";
                    }

                    for (size_t i = AZ::eBehaviorBusForwarderEventIndices::ParameterFirst; i < event.m_parameters.size(); ++i)
                    {
                        const AZ::BehaviorParameter& argParam = event.m_parameters[i];
                        dumpStr += ",";
                        AZStd::string argType = argParam.m_name;
                        dumpStr += "\"" + argType;

                        AZStd::string argName = argType;
                        AZStd::string argToolTip = argType;

                        if (event.m_metadataParameters.size() > i)
                        {
                            argName = event.m_metadataParameters[i].m_name;
                            argToolTip = event.m_metadataParameters[i].m_toolTip;
                        }

                        if (!argName.empty())
                        {
                            dumpStr += "(" + argName + ")";
                            if (!argToolTip.empty())
                            {
                                dumpStr += ": " + argToolTip;
                            }
                        }

                        dumpStr += "\"";
                    }

                    for (size_t i = event.m_parameters.size(); i < DumpInternal::maxInputParameters; ++i)
                    {
                        dumpStr += ",";
                    }
                    dumpStr += "\n";
                }

                ebus->m_destroyHandler->Invoke(handler); // Destroys the Created EbusHandler
            }
        }

        void DumpBehaviorContextEbuses(AZStd::string& dumpStr)
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            AZ::BehaviorContext* behaviorContext{};
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

            if (serializeContext == nullptr || behaviorContext == nullptr)
            {
                return;
            }


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

                // Check for "ignore" attribute for ScriptCanvas
                auto excludeClassAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));
                const bool excludeClass = excludeClassAttributeData && static_cast<AZ::u64>(excludeClassAttributeData->Get(nullptr)) & static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);
                if (excludeClass)
                {
                    for (const auto& requestBus : behaviorClass->m_requestBuses)
                    {
                        skipBuses.insert(AZ::Crc32(requestBus.c_str()));
                    }
                    continue; // skip this class
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
                AZ::BehaviorEBus* ebus = ebusIter.second;

                if (ebus == nullptr)
                {
                    continue;
                }

                auto excludeEbusAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, ebusIter.second->m_attributes));
                const bool excludeBus = excludeEbusAttributeData && static_cast<AZ::u64>(excludeEbusAttributeData->Get(nullptr)) & static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);

                auto skipBusIterator = skipBuses.find(AZ::Crc32(ebusIter.first.c_str()));
                if (skipBusIterator != skipBuses.end() || excludeBus)
                {
                    continue;
                }

                AZStd::string_view categoryName;
                if (auto categoryAttribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::Category, ebus->m_attributes)))
                {
                    categoryName = categoryAttribute->Get(nullptr);
                }

                DumpBehaviorContextEBusHandlers(dumpStr, ebus, categoryName);

                for (const auto& eventIter : ebus->m_events)
                {

                    auto excludeEventAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, eventIter.second.m_attributes));
                    const bool excludeEvent = excludeEventAttributeData && static_cast<AZ::u64>(excludeEventAttributeData->Get(nullptr)) & static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);
                    if (excludeEvent)
                    {
                        continue;
                    }

                    const AZ::BehaviorMethod* const method = eventIter.second.m_event ? eventIter.second.m_event : eventIter.second.m_broadcast;
                    if (!method)
                    {
                        continue;
                    }

                    dumpStr += AZStd::string::format(R"("%s","%s","%s",)", !categoryName.empty() ? categoryName.data() : "", ebusIter.first.c_str(), eventIter.first.c_str());
                    const auto& resultParam = method->HasResult() ? method->GetResult() : nullptr;
                    if (resultParam)
                    {
                        AZStd::string resultType = resultParam->m_name;
                        dumpStr += "\"" + resultType + "\"";
                    }
                    else
                    {
                        dumpStr += "\"void\"";
                    }

                    for (size_t i = 0; i < method->GetNumArguments(); ++i)
                    {
                        dumpStr += ",";
                        if (const auto& argParam = method->GetArgument(i))
                        {
                            AZStd::string argType = argParam->m_name;
                            dumpStr += "\"" + argType;
                            const AZStd::string* argName = method->GetArgumentName(i);
                            if (argName && !argName->empty())
                            {
                                dumpStr += "(" + *argName + ")";
                                const AZStd::string* argToolTip = method->GetArgumentToolTip(i);
                                if (argToolTip && !argToolTip->empty())
                                {
                                    dumpStr += ": " + *argToolTip;
                                }
                            }
                            else if (i == 0 && method->HasBusId())
                            {
                                dumpStr += "(EBus ID)";
                            }

                            dumpStr += "\"";
                        }
                    }

                    for (size_t i = method->GetNumArguments(); i < DumpInternal::maxInputParameters; ++i)
                    {
                        dumpStr += ",";
                    }
                    dumpStr += "\n";
                }
            }
        }
    }
}
