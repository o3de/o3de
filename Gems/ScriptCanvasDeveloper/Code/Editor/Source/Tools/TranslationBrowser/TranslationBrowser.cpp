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

#include "TranslationBrowser.h"

#include <QCompleter>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QScrollBar>
#include <QDateTime>
#include <QToolButton>
#include <QListView>
#include <QStringListModel>
#include <QPushButton>

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Editor/Settings.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>

#include <Editor/Translation/TranslationHelper.h>

#include <Source/Translation/TranslationAsset.h>
#include <Source/Translation/TranslationSerializer.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <Tools/TranslationBrowser/ui_TranslationBrowser.h>

namespace ScriptCanvasDeveloper
{

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
                outName = classData->m_editData->m_name ? classData->m_editData->m_name : "";
                outDescription = classData->m_editData->m_description ? classData->m_editData->m_description : "";
            }
            else
            {
                outName = classData->m_name;
            }
        }
    }



    BehaviorClassModel::BehaviorClassModel()
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

        for (auto& behaviorIt : behaviorContext->m_classes)
        {
            AZ::BehaviorClass* behaviorClass = behaviorIt.second;
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

            auto classItem = std::make_shared<TreeNode>(className.c_str(), "BehaviorClass", behaviorClass);
            
            m_topLevelItems.push_back(classItem);
        }

        PopulateScriptCanvasNodes();
    }

    void BehaviorClassModel::PopulateScriptCanvasNodes()
    {
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        //VerificationSet verificationSet;
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
                AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(classData->m_name);

                auto classItem = std::make_shared<TreeNode>(cleanName.c_str(), "ScriptCanvas::Node", classData);

                m_topLevelItems.push_back(classItem);
            }
        }
    }

    TranslationBrowser::TranslationBrowser(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDialog(parent)
        , m_ui(new Ui::TranslationBrowser())
    {
        m_ui->setupUi(this);

        // Get contexts
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ::ComponentApplicationBus::BroadcastResult(m_behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

        m_ui->searchWidget->SetFilterInputInterval(AZStd::chrono::milliseconds(250));
        QObject::connect(m_ui->searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &TranslationBrowser::OnFilterChanged);


        m_behaviorContextClassesModel = new BehaviorClassModel();
        m_proxyModel = new BehaviorClassModelSortFilterProxyModel(m_behaviorContextClassesModel);
        

        m_ui->localizationDataListView->setModel(m_proxyModel);

        connect(m_ui->localizationDataListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TranslationBrowser::OnSelectionChanged);
        connect(m_ui->localizationDataListView, &QAbstractItemView::doubleClicked, this, &TranslationBrowser::OnDoubleClick);

        connect(m_ui->btnSaveSource, &QPushButton::clicked, this, &TranslationBrowser::SaveSource);
        connect(m_ui->btnSaveOverride, &QPushButton::clicked, this, &TranslationBrowser::SaveOverride);
        connect(m_ui->btnGenerateData, &QPushButton::clicked, this, &TranslationBrowser::Generate);
        connect(m_ui->btnDumpDatabase, &QPushButton::clicked, this, &TranslationBrowser::DumpDatabase);
        connect(m_ui->btnOpenInExplorer, &QPushButton::clicked, this, &TranslationBrowser::ShowOverrideInExplorer);
        connect(m_ui->btnReload, &QPushButton::clicked, this, &TranslationBrowser::ReloadDatabase);

        Populate();


    }

    void TranslationBrowser::SaveSource()
    {

    }

    void TranslationBrowser::SaveOverride()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "FileIO is not initialized.");
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;

        if (const AZ::IO::Result result = fileIO->Open(m_selection.c_str(), AZ::IO::OpenMode::ModeWrite, fileHandle))
        {
            AZStd::string contents = m_ui->fromFileTranslationData->toPlainText().toUtf8().data();
            fileIO->Write(fileHandle, contents.c_str(), contents.size());
            fileIO->Close(fileHandle);
        }
    }

    void TranslationBrowser::Generate()
    {
        ::ScriptCanvasDeveloperEditor::TranslationGenerator::GenerateTranslationDatabase();
    }

    void TranslationBrowser::DumpDatabase()
    {
        GraphCanvas::TranslationRequestBus::Broadcast(&GraphCanvas::TranslationRequests::DumpDatabase, "@user@/ScriptCanvas/Translations/database.log");
    }

    void TranslationBrowser::ShowOverrideInExplorer()
    {
        if (!m_selection.empty())
        {
            AzQtComponents::ShowFileOnDesktop(m_selection.c_str());
        }
    }

    void TranslationBrowser::ReloadDatabase()
    {
        GraphCanvas::TranslationRequestBus::Broadcast(&GraphCanvas::TranslationRequests::Restore);
    }

    TranslationBrowser::~TranslationBrowser()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
    }


    void TranslationBrowser::OnFilterChanged(const QString& filterString)
    {
        m_proxyModel->SetInput(filterString.toUtf8().data());
    }

    void TranslationBrowser::Populate()
    {
        PopulateBehaviorContextClasses();
    }

    void TranslationBrowser::PopulateScriptCanvasNodes()
    {
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        //VerificationSet verificationSet;
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
                //Entry entry;
                //entry.m_key = classData->m_typeId.ToString<AZStd::string>();
                //entry.m_context = "ScriptCanvas::Node";

                //EntryDetails& details = entry.m_details;

                AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(classData->m_name);
                //details.m_name = cleanName;

                // Tooltip attribute takes priority over the edit data description
                AZStd::string tooltip = GraphCanvasAttributeHelper::GetStringAttribute(classData, AZ::Script::Attributes::ToolTip);
                if (!tooltip.empty())
                {
                    //details.m_tooltip = tooltip;
                }
                else
                {
                    //details.m_tooltip = classData->m_editData ? classData->m_editData->m_description : "";
                }

                // Find the category
                //details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(classData, AZ::Script::Attributes::Category);
                if (/*details.m_category.empty() && */classData->m_editData)
                {
                    auto elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                    const AZStd::string categoryAttribute = GraphCanvasAttributeHelper::ReadStringAttribute(elementData->m_attributes, AZ::Script::Attributes::Category);
                    if (!categoryAttribute.empty())
                    {
                        //details.m_category = categoryAttribute;
                    }
                }

                if (ScriptCanvas::Node* nodeComponent = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name)))
                {
                    nodeComponent->Configure();

                    [[maybe_unused]] int inputIndex = 0;
                    [[maybe_unused]] int outputIndex = 0;

                    const auto& allSlots = nodeComponent->GetAllSlots();
                    for ([[maybe_unused]] const auto& slot : allSlots)
                    {
                        /*Slot slotEntry;

                        if (slot->GetDescriptor().IsExecution())
                        {
                            if (slot->GetDescriptor().IsInput())
                            {
                                slotEntry.m_key = AZStd::string::format("Input_%d", inputIndex);
                                inputIndex++;

                                slotEntry.m_details.m_name = slot->GetName();
                                slotEntry.m_details.m_tooltip = slot->GetToolTip();
                            }
                            else if (slot->GetDescriptor().IsOutput())
                            {
                                slotEntry.m_key = AZStd::string::format("Output_%d", outputIndex);
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
                                slotEntry.m_key = AZStd::string::format("DataInput_%d", inputIndex);
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
                                slotEntry.m_key = AZStd::string::format("DataOutput_%d", outputIndex);
                                outputIndex++;

                                AZStd::string resultKey = slotTypeKey;
                                AZStd::string resultName = slot->GetName();
                                AZStd::string resultDescription = slot->GetToolTip();

                                argument.m_typeId = resultKey;
                                argument.m_details.m_name = resultName;
                                argument.m_details.m_tooltip = resultDescription;
                            }

                            entry.m_slots.push_back(slotEntry);
                        }*/
                    }

                    delete nodeComponent;
                }

                //translationRoot.m_entries.push_back(entry);
            }
        }
    }

    void TranslationBrowser::PopulateBehaviorContextClasses()
    {
        QStringList classesStringList;
        for (const auto& classIter : m_behaviorContext->m_classes)
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

            classesStringList.push_back(className.c_str());

        }

        //m_behaviorContextClassesModel->setStringList(classesStringList);
        m_behaviorContextClassesModel->sort(0);

            //m_ui->localizationDataListView->

        //    {
        //        TranslationFormat translationRoot;

        //        Entry entry;

        //        EntryDetails& details = entry.m_details;
        //        details.m_name = className;
        //        details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
        //        details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::ToolTip);
        //        entry.m_context = "";
        //        entry.m_key = behaviorClass->m_name;

        //        if (!behaviorClass->m_methods.empty())
        //        {
        //            for (const auto& methodPair : behaviorClass->m_methods)
        //            {
        //                const AZ::BehaviorMethod* behaviorMethod = methodPair.second;

        //                Method methodEntry;

        //                AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodPair.first);

        //                methodEntry.m_key = cleanName;
        //                methodEntry.m_context = className;

        //                methodEntry.m_details.m_category = "";
        //                methodEntry.m_details.m_tooltip = "";
        //                methodEntry.m_details.m_name = methodPair.second->m_name;

        //                methodEntry.m_entry.m_name = "In";
        //                methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", methodEntry.m_details.m_name.c_str());
        //                methodEntry.m_exit.m_name = "Out";
        //                methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", methodEntry.m_details.m_name.c_str());

        //                // Arguments (Input Slots)
        //                if (behaviorMethod->GetNumArguments() > 0)
        //                {
        //                    for (size_t argIndex = 0; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
        //                    {
        //                        const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

        //                        Argument argument;

        //                        AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
        //                        AZStd::string argumentName = parameter->m_name;
        //                        AZStd::string argumentDescription = "";

        //                        GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

        //                        argument.m_typeId = argumentKey;
        //                        argument.m_details.m_name = argumentName;
        //                        argument.m_details.m_category = "";
        //                        argument.m_details.m_tooltip = argumentDescription;

        //                        methodEntry.m_arguments.push_back(argument);
        //                    }
        //                }

        //                const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
        //                if (resultParameter)
        //                {
        //                    Argument result;

        //                    AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

        //                    AZStd::string resultName = resultParameter->m_name;
        //                    AZStd::string resultDescription = "";

        //                    GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

        //                    result.m_typeId = resultKey;
        //                    result.m_details.m_name = resultName;
        //                    result.m_details.m_tooltip = resultDescription;

        //                    methodEntry.m_results.push_back(result);
        //                }

        //                entry.m_methods.push_back(methodEntry);
        //            }
        //        }

        //        translationRoot.m_entries.push_back(entry);

        //        AZStd::string fileName = AZStd::string::format("Classes/%s", className.c_str());

        //        SaveJSONData(fileName, translationRoot);
        //    }
        //}
    }


    void TranslationBrowser::OnSelectionChanged()
    {
        const QModelIndex item = m_ui->localizationDataListView->currentIndex();
        if (!item.isValid() )
        {
            m_ui->sourceTranslationData->clear();
        }
        else
        {

            m_ui->sourceTranslationData->clear();

            if (m_translationMode == TranslationMode::BehaviorClass)
            {
                auto childNode = item.data(BehaviorClassModel::DataRoles::BehaviorClass).value<BehaviorClassModel::TreeNode>();

                if (childNode.m_behaviorClass)
                {
                    AZ::BehaviorClass* behaviorClass = childNode.m_behaviorClass;
                    ShowBehaviorClass(behaviorClass);
                }
                else if (childNode.m_classData)
                {
                    ShowClassData(childNode.m_classData);
                }
            }
        }



    }

    bool MethodHasAttribute(const AZ::BehaviorMethod* method, AZ::Crc32 attribute)
    {
        return AZ::FindAttribute(attribute, method->m_attributes) != nullptr; // warning C4800: 'AZ::Attribute *': forcing value to bool 'true' or 'false' (performance warning)
    }

    void TranslationBrowser::ShowClassData(const AZ::SerializeContext::ClassData* classData)
    {
        using namespace ScriptCanvasDeveloperEditor::TranslationGenerator;

        AZStd::string className = classData->m_name;

        LoadJSONForClass(className.c_str());

        m_ui->sourceTranslationData->setPlainText(className.c_str());
    }

    void TranslationBrowser::ShowBehaviorClass(AZ::BehaviorClass* behaviorClass)
    {
        using namespace ScriptCanvasDeveloperEditor::TranslationGenerator;

        AZStd::string className = behaviorClass->m_name;

        LoadJSONForClass(className);
        
        {
            AZStd::string prettyName = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::ScriptCanvasAttributes::PrettyName);
            if (!prettyName.empty())
            {
                className = prettyName;
            }

            TranslationFormat translationRoot;

            {
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

                auto translatedName = ScriptCanvasEditor::TranslationHelper::GetClassKeyTranslation(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, className, ScriptCanvasEditor::TranslationKeyId::Name);
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
            }

            AZStd::string content = MakeJSON(translationRoot);
            m_ui->sourceTranslationData->setPlainText(content.c_str());
        }
    }

    void TranslationBrowser::OnDoubleClick()
    {

    }

    void TranslationBrowser::OnSystemTick()
    {
    }



    void TranslationBrowser::closeEvent(QCloseEvent* event)
    {

        AzQtComponents::StyledDialog::closeEvent(event);
    }

    void TranslationBrowser::LoadJSONForClass(const AZStd::string& className)
    {
      
        AZStd::string fileName = AZStd::string::format("%s.names", className.c_str());
        fileName = GraphCanvas::TranslationKey::Sanitize(fileName);
        AZStd::to_lower(fileName.begin(), fileName.end());

        AZStd::string found;

        // Find any TranslationAsset files that may have translation database key/values
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr,
            [fileName, &found](const AZ::Data::AssetId /*assetId*/, const AZ::Data::AssetInfo& assetInfo) {
                const auto assetType = azrtti_typeid<GraphCanvas::TranslationAsset>();
                if (found.empty() && assetInfo.m_assetType == assetType)
                {
                    AZStd::string testFile;
                    AZ::StringFunc::Path::GetFullFileName(assetInfo.m_relativePath.c_str(), testFile);
                    testFile = GraphCanvas::TranslationKey::Sanitize(testFile);
                    AZStd::to_lower(testFile.begin(), testFile.end());

                    if (fileName == testFile)
                    {
                        found = assetInfo.m_relativePath;
                    }
                }
            },
            []() {
                
            }
            );


        if (!found.empty() && AZ::IO::FileIOBase::GetInstance()->Exists(found.c_str()))
        {
            m_ui->btnOpenInExplorer->setEnabled(true);
            m_selection = found;

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "FileIO is not initialized.");
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;

            if (const AZ::IO::Result result = fileIO->Open(found.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle))
            {
                AZ::u64 sizeBytes = 0;
                if (fileIO->Size(fileHandle, sizeBytes))
                {
                    AZStd::unique_ptr<char[]> buffer = AZStd::make_unique<char[]>(sizeBytes + 1);
                    if (fileIO->Read(fileHandle, buffer.get(), sizeBytes))
                    {
                        fileIO->Close(fileHandle);
                        buffer[sizeBytes] = '\0';

                        m_ui->fromFileTranslationData->clear();
                        m_ui->fromFileTranslationData->setPlainText(buffer.get());
                    }
                }
            }

        }
        else
        {
            m_ui->btnOpenInExplorer->setEnabled(false);
        }

    }

    AZStd::string TranslationBrowser::MakeJSON(const ScriptCanvasDeveloperEditor::TranslationGenerator::TranslationFormat& translationRoot)
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

            entries.PushBack(entry, document.GetAllocator());
        }

        document.AddMember("entries", entries, document.GetAllocator());

        rapidjson::StringBuffer scratchBuffer;

        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(scratchBuffer);
        document.Accept(writer);

        AZStd::string jsonOutput = scratchBuffer.GetString();

        scratchBuffer.Clear();

        return jsonOutput;
    }

    BehaviorClassModelSortFilterProxyModel::BehaviorClassModelSortFilterProxyModel(BehaviorClassModel* behaviorClassModel, QObject* parent /*= nullptr*/)
        : QSortFilterProxyModel(parent)
    {
        setSourceModel(behaviorClassModel);

        QStringList nameList;

        for (int i = 0; i < behaviorClassModel->Count(); ++i)
        {
            QModelIndex index = behaviorClassModel->index(i, 0);
            QString name = behaviorClassModel->data(index, 0).toString();
            nameList.push_back(name);
        }

    }

    bool BehaviorClassModelSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        BehaviorClassModel* dataModel = qobject_cast<BehaviorClassModel*>(sourceModel());

        if (m_input.empty())
        {
            return true;
        }

        if (sourceRow < 0 || sourceRow >= dataModel->rowCount(sourceParent))
        {
            return false;
        }

        QModelIndex index = dataModel->index(sourceRow, BehaviorClassModel::DataRoles::BehaviorClass);

        auto sourceStr  = dataModel->data(index).toString();

        if (sourceRow >= 0 && sourceStr.startsWith(m_input.c_str(), Qt::CaseSensitivity::CaseInsensitive))
        {
            return true;
        }

        return false;
    }

    TranslationHeaderView::TranslationHeaderView(QWidget* parent /*= nullptr*/)
        : QTableView(parent)
    {
        horizontalHeader()->setStretchLastSection(false);

        horizontalHeader()->show();

        setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        horizontalHeader()->setSortIndicatorShown(false);

    }

}
