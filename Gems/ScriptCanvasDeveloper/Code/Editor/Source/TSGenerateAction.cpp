/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvasDeveloperEditor/TSGenerateAction.h>
#include <XMLDoc.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>

namespace ScriptCanvasDeveloperEditor
{
    namespace TSGenerateAction
    {
        void GenerateTSFile();
        void DumpBehaviorContextMethods(const XMLDocPtr& doc);
        void DumpBehaviorContextEbuses(const XMLDocPtr& doc);
        void DumpBehaviorContextEBusHandlers(const XMLDocPtr& doc, AZ::BehaviorEBus* ebus, const AZStd::string& categoryName);
        bool StartContext(const XMLDocPtr& doc, const AZStd::string& contextType, const AZStd::string& contextName, const AZStd::string& toolTip, const AZStd::string& categoryName, bool addContextTypeToKey= false);
        void AddMessageNode(const XMLDocPtr& doc, const AZStd::string& classorbusName, const AZStd::string& eventormethodName, const AZStd::string& toolTip, const AZStd::string& categoryName, const AZ::BehaviorEBusHandler::BusForwarderEvent& event);
        void AddMessageNode(const XMLDocPtr& doc, const AZStd::string& classorbusName, const AZStd::string& eventormethodName, const AZStd::string& toolTip, const AZStd::string& categoryName, const AZ::BehaviorMethod* method);

        QAction* SetupTSFileAction(QMenu* mainMenu)
        {
            QAction* qAction = nullptr;

            if (mainMenu)
            {
                qAction = mainMenu->addAction(QAction::tr("Create EBus Localization File"));
                qAction->setAutoRepeat(false);
                qAction->setToolTip("Creates a QT .TS file of all EBus nodes(their inputs and outputs) to a file in the current folder.");
                qAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Alt+X", "Debug|Build EBus .TS file")));

                QObject::connect(qAction, &QAction::triggered, &GenerateTSFile);
            }

            return qAction;
        }

        void GenerateTSFile()
        {
            auto translationScriptPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) /
                "Assets" / "Editor" / "Translation" / "scriptcanvas_en_us.ts";

            XMLDocPtr tsDoc(XMLDoc::LoadFromDisk(translationScriptPath.c_str()));

            if (tsDoc == nullptr)
            {
                tsDoc = XMLDoc::Alloc("ScriptCanvas");
            }

            DumpBehaviorContextMethods(tsDoc);
            DumpBehaviorContextEbuses(tsDoc);

            tsDoc->WriteToDisk(translationScriptPath.c_str());
        }

        void DumpBehaviorContextMethods(const XMLDocPtr& doc)
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

                AZStd::string categoryName;
                if (auto categoryAttribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::Category, behaviorClass->m_attributes)))
                {
                    categoryName = categoryAttribute->Get(nullptr);
                }

                AZStd::string methodToolTip;
                if (auto methodToolTipAttribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::ToolTip, behaviorClass->m_attributes)))
                {
                    methodToolTip = methodToolTipAttribute->Get(nullptr);
                }

                bool addContext = false;

                for (auto methodPair : behaviorClass->m_methods)
                {
                    // Check for "ignore" attribute for ScriptCanvas
                    auto excludeMethodAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, methodPair.second->m_attributes));
                    const bool excludeMethod = excludeMethodAttributeData && static_cast<AZ::u64>(excludeMethodAttributeData->Get(nullptr)) & static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);
                    if (excludeMethod)
                    {
                        continue; // skip this method
                    }

                    if( !addContext )
                    {
                        StartContext(doc, "Method", classIter.first, methodToolTip, categoryName);
                        addContext = true;
                    }

                    AZStd::string toolTip;
                    if (auto toolTipAttribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::ToolTip, methodPair.second->m_attributes)))
                    {
                        toolTip = toolTipAttribute->Get(nullptr);
                    }

                    AZStd::string nodeCategoryName;
                    if (auto attribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::Category, methodPair.second->m_attributes)))
                    {
                        nodeCategoryName = attribute->Get(nullptr);
                    }

                    AddMessageNode(doc, classIter.first, methodPair.first, toolTip, nodeCategoryName, methodPair.second);
                }
            }
        }

        void DumpBehaviorContextEbuses(const XMLDocPtr& doc)
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
                bool addContext = false;
                AZ::BehaviorEBus* ebus = ebusIter.second;

                if (ebus == nullptr)
                {
                    continue;
                }

                auto excludeEbusAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, ebusIter.second->m_attributes));
                const bool excludeBus = excludeEbusAttributeData && static_cast<AZ::u64>(excludeEbusAttributeData->Get(nullptr)) & static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);

                auto skipBusIterator = skipBuses.find(AZ::Crc32(ebusIter.first.c_str()));
                if (!ebus || skipBusIterator != skipBuses.end() || excludeBus)
                {
                    continue;
                }

                AZStd::string categoryName;
                if (auto categoryAttribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::Category, ebus->m_attributes)))
                {
                    auto categoryAttribName = categoryAttribute->Get(nullptr);

                    if (categoryAttribName != nullptr)
                    {
                        categoryName = categoryAttribName;
                    }
                }

                DumpBehaviorContextEBusHandlers(doc, ebus, categoryName);

                for (const auto& eventIter : ebus->m_events)
                {
                    const AZ::BehaviorMethod* const method = (eventIter.second.m_event != nullptr) ? eventIter.second.m_event : eventIter.second.m_broadcast;
                    if (!method || AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, eventIter.second.m_attributes))
                    {
                        continue;
                    }

                    if( !addContext )
                    {
                        StartContext(doc, "EBus", ebusIter.first, ebusIter.second->m_toolTip, categoryName);
                        addContext = true;
                    }

                    AZStd::string toolTip;
                    if (auto toolTipAttribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::ToolTip, eventIter.second.m_attributes)))
                    {
                        toolTip = toolTipAttribute->Get(nullptr);
                    }

                    AZStd::string nodeCategoryName;
                    if (auto attribute = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(AZ::Script::Attributes::Category, eventIter.second.m_attributes)))
                    {
                        nodeCategoryName = attribute->Get(nullptr);
                    }

                    AddMessageNode(doc, ebusIter.first, eventIter.first, toolTip, nodeCategoryName, method);
                }
            }
        }

        void DumpBehaviorContextEBusHandlers(const XMLDocPtr& doc, AZ::BehaviorEBus* ebus, const AZStd::string& categoryName)
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

            bool addContext = false;

            AZ::BehaviorEBusHandler* handler(nullptr);
            if (ebus->m_createHandler->InvokeResult(handler))
            {
                for (const AZ::BehaviorEBusHandler::BusForwarderEvent& event : handler->GetEvents())
                {
                    if (!addContext)
                    {
                        StartContext(doc, "Handler", ebus->m_name, ebus->m_toolTip, categoryName, true);
                        addContext = true;
                    }

                    AddMessageNode(doc, ebus->m_name, event.m_name, "", categoryName, event);
                }

                ebus->m_destroyHandler->Invoke(handler); // Destroys the Created EbusHandler
            }
        }

        AZStd::string GetBaseID(const AZStd::string& classorbusName, const AZStd::string& eventormethodName)
        {
            AZStd::string p1(classorbusName);
            AZStd::string p2(eventormethodName);

            AZStd::to_upper(p1.begin(), p1.end());
            AZStd::to_upper(p2.begin(), p2.end());

            return p1 + "_" + p2;
        }

        void AddCommonNodeElements(const XMLDocPtr& doc, const AZStd::string& baseID, const AZStd::string& classorbusName, const AZStd::string& eventormethodName, const AZStd::string& toolTip, const AZStd::string& categoryName)
        {
            doc->AddToContext(baseID + "_NAME", eventormethodName, AZStd::string::format("Class/Bus: %s  Event/Method: %s", classorbusName.c_str(), eventormethodName.c_str()));
            doc->AddToContext(baseID + "_TOOLTIP", toolTip);
            doc->AddToContext(baseID + "_CATEGORY", categoryName);
            doc->AddToContext(baseID + "_OUT_NAME");
            doc->AddToContext(baseID + "_OUT_TOOLTIP");
            doc->AddToContext(baseID + "_IN_NAME");
            doc->AddToContext(baseID + "_IN_TOOLTIP");
        }

        void AddResultElements(const XMLDocPtr& doc, const AZStd::string& baseID, const AZ::Uuid& typeId, const AZStd::string& name, const AZStd::string& toolTip)
        {
            ScriptCanvas::Data::Type outputType(ScriptCanvas::Data::FromAZType(typeId));

            doc->AddToContext(baseID + "_OUTPUT0_NAME", ScriptCanvas::Data::GetName(outputType), "C++ Type: " + name);
            doc->AddToContext(baseID + "_OUTPUT0_TOOLTIP", toolTip);
        }

        void AddParameterElements(const XMLDocPtr& doc, const AZStd::string& baseID, size_t index, const AZ::Uuid& typeId, const AZStd::string& argName, const AZStd::string& argToolTip, const AZStd::string& cppType)
        {
            AZStd::string paramID(AZStd::string::format("%s_PARAM%zu_", baseID.c_str(), index));

            ScriptCanvas::Data::Type outputType(ScriptCanvas::Data::FromAZType(typeId));

            doc->AddToContext(paramID + "NAME", argName, AZStd::string::format("Simple Type: %s C++ Type: %s", ScriptCanvas::Data::GetName(outputType).c_str(), cppType.c_str()));
            doc->AddToContext(paramID + "TOOLTIP", argToolTip);
        }

        void AddOutputElements(const XMLDocPtr& doc, const AZStd::string& baseID, size_t index, const AZ::Uuid& typeId, const AZStd::string& argName, const AZStd::string& argToolTip, const AZStd::string& cppType)
        {
            AZStd::string paramID(AZStd::string::format("%s_OUTPUT%zu_", baseID.c_str(), index));

            ScriptCanvas::Data::Type outputType(ScriptCanvas::Data::FromAZType(typeId));

            doc->AddToContext(paramID + "NAME", argName, AZStd::string::format("Simple Type: %s C++ Type: %s", ScriptCanvas::Data::GetName(outputType).c_str(), cppType.c_str()));
            doc->AddToContext(paramID + "TOOLTIP", argToolTip);
        }

        bool StartContext(const XMLDocPtr& doc, const AZStd::string& contextType, const AZStd::string& contextName, const AZStd::string& toolTip, const AZStd::string& categoryName, bool addContextTypeToKey/* = false*/)
        {
            bool isNewContext = doc->StartContext(contextType + ": " + contextName);

            if( isNewContext )
            {
                AZStd::string p1(contextName);

                if(addContextTypeToKey)
                {
                    p1 = contextType + "_" + p1;
                }

                p1 += "_";

                AZStd::to_upper(p1.begin(), p1.end());

                doc->AddToContext(p1 + "NAME", contextName);
                doc->AddToContext(p1 + "TOOLTIP", toolTip);
                doc->AddToContext(p1 + "CATEGORY", categoryName);
            }

            return isNewContext;
        }

        void AddMessageNode(const XMLDocPtr& doc, const AZStd::string& classorbusName, const AZStd::string& eventormethodName, const AZStd::string& toolTip, const AZStd::string& categoryName, const AZ::BehaviorEBusHandler::BusForwarderEvent& event)
        {
            AZStd::string baseID( "HANDLER_" + GetBaseID(classorbusName, eventormethodName));

            if( !doc->MethodFamilyExists(baseID) )
            {
                AddCommonNodeElements(doc, baseID, classorbusName, eventormethodName, toolTip, categoryName);

                if ( event.HasResult() )
                {
                    const AZStd::string name = event.m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::Result].m_name.empty() ? event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result].m_name : event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result].m_name;

                    AddParameterElements(doc, baseID, 0, event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result].m_typeId, name, event.m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::Result].m_toolTip, "");

                    AZ_TracePrintf("ScriptCanvas", "EBusHandler Index: 0 CategoryName: %s Ebus: %s Event: %s Name: %s", categoryName.c_str(), classorbusName.c_str(), eventormethodName.c_str(), name.c_str());
                }

                size_t outputIndex = 0;
                for (size_t i = AZ::eBehaviorBusForwarderEventIndices::ParameterFirst; i < event.m_parameters.size(); ++i)
                {
                    const AZ::BehaviorParameter& argParam = event.m_parameters[i];

                    AddOutputElements(doc, baseID, outputIndex++, argParam.m_typeId, event.m_metadataParameters[i].m_name, event.m_metadataParameters[i].m_toolTip, argParam.m_name);
                }
            }
        }

        void AddMessageNode(const XMLDocPtr& doc, const AZStd::string& classorbusName, const AZStd::string& eventormethodName, const AZStd::string& toolTip, const AZStd::string& categoryName, const AZ::BehaviorMethod* method)
        {
            AZStd::string baseID( GetBaseID(classorbusName, eventormethodName) );

            if (!doc->MethodFamilyExists(baseID))
            {
                AddCommonNodeElements(doc, baseID, classorbusName, eventormethodName, toolTip, categoryName);

                const auto result = method->HasResult() ? method->GetResult() : nullptr;
                if (result)
                {
                    AddResultElements(doc, baseID, result->m_typeId, result->m_name, "");
                }

                size_t start = method->HasBusId() ? 1 : 0;
                for (size_t i = start; i < method->GetNumArguments(); ++i)
                {
                    if (const AZ::BehaviorParameter* argument = method->GetArgument(i))
                    {
                        AddParameterElements(doc, baseID, i-start, argument->m_typeId, *method->GetArgumentName(i), *method->GetArgumentToolTip(i), argument->m_name);
                    }
                }
            }
        }
    }
}
