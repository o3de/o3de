/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qaction.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qheaderview.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Utils/QtMimeUtils.h>
#include <GraphCanvas/Widgets/StyledItemDelegates/GenericComboBoxDelegate.h>

#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/QtMetaTypes.h>

#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

namespace ScriptCanvasEditor
{

    const char* GraphVariablesModel::m_columnNames[static_cast<int>(ColumnIndex::Count)] =
    {
        "Name",
        "Type",
        "Default Value",
        "Scope",
        "Initial Value"
    };

    ////////////////////////
    // GraphVariablesModel
    ////////////////////////

    GraphVariablesModel::GraphVariablesModel(QObject* parent /*= nullptr*/)
        : QAbstractTableModel(parent)
    {
    }

    GraphVariablesModel::~GraphVariablesModel()
    {
        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
    }

    int GraphVariablesModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
    {
        return ColumnIndex::Count;
    }

    int GraphVariablesModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
    {
        return aznumeric_cast<int>(m_variableIds.size());
    }

    QVariant GraphVariablesModel::data(const QModelIndex &index, int role) const
    {
        ScriptCanvas::GraphScopedVariableId varId = FindScopedVariableIdForIndex(index);

        switch (role)
        {
        case VarIdRole:
            return QVariant::fromValue<ScriptCanvas::VariableId>(varId.m_identifier);

        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                AZStd::string_view title;
                ScriptCanvas::VariableRequestBus::EventResult(title, varId, &ScriptCanvas::VariableRequests::GetName);

                AZStd::string string = title;
                return QString(string.c_str());
            }
            else if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable && IsEditableType(varType))
                {
                    if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                    {
                        const ScriptCanvas::Data::StringType* stringValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::StringType>();

                        return QVariant(stringValue->c_str());
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Number()))
                    {
                        const ScriptCanvas::Data::NumberType* numberValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::NumberType>();

                        return QVariant((*numberValue));
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                    {
                        const ScriptCanvas::Data::BooleanType* booleanValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::BooleanType>();

                        return QVariant((*booleanValue));
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::CRC()))
                    {
                        const ScriptCanvas::Data::CRCType* crcValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::CRCType>();

                        AZStd::string crcString;
                        EditorGraphRequestBus::EventResult(crcString, GetScriptCanvasId(), &EditorGraphRequests::DecodeCrc, (*crcValue));
                        return QVariant(crcString.c_str());
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "Unhandled editable type found in GraphVariablesTableView.cpp");
                    }
                }
            }
            else if (index.column() == ColumnIndex::Scope)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable)
                {
                    return ScriptCanvas::VariableFlags::GetScopeDisplayLabel(graphVariable->GetScope());
                }
            }
            else if (index.column() == ColumnIndex::InitialValueSource)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable)
                {
                    return graphVariable->GetInitialValueSourceName().data();
                }
            }
        }
        break;

        case Qt::DisplayRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                AZStd::string_view title;

                ScriptCanvas::VariableRequestBus::EventResult(title, varId, &ScriptCanvas::VariableRequests::GetName);

                return QString::fromUtf8(title.data(), static_cast<int>(title.size()));
            }
            else if (index.column() == ColumnIndex::Type)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                if (varType.IsValid())
                {
                    AZStd::string type = TranslationHelper::GetSafeTypeName(varType);

                    return QString::fromUtf8(type.data(), static_cast<int>(type.size()));
                }

                return QVariant();
            }
            else if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable && IsEditableType(varType))
                {
                    if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                    {
                        const ScriptCanvas::Data::StringType* stringValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::StringType>();

                        if (stringValue->empty())
                        {
                            return QString("<None>");
                        }
                        else
                        {
                            return QString(stringValue->c_str());
                        }
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::CRC()))
                    {
                        const ScriptCanvas::Data::CRCType* crcValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::CRCType>();

                        AZStd::string crcString;
                        EditorGraphRequestBus::EventResult(crcString, GetScriptCanvasId(), &EditorGraphRequests::DecodeCrc, (*crcValue));

                        if (!crcString.empty())
                        {
                            return QString::fromUtf8(crcString.c_str(), static_cast<int>(crcString.size()));
                        }
                        else
                        {
                            return QString("<Empty>");
                        }
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Number()))
                    {
                        const ScriptCanvas::Data::NumberType* numberValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::NumberType>();

                        return QVariant((*numberValue));
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                    {
                        // Want to return nothing for the boolean, because we'll just use the check box
                        return QVariant();
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "Unhandled editable type found in GraphVariablesTableView.cpp");
                    }
                }

                return QVariant();
            }
            else if (index.column() == ColumnIndex::Scope)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable)
                {
                    return ScriptCanvas::VariableFlags::GetScopeDisplayLabel(graphVariable->GetScope());
                }
            }
            else if (index.column() == ColumnIndex::InitialValueSource)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable)
                {
                    return graphVariable->GetInitialValueSourceName().data();
                }
            }
        }
        break;

        case Qt::FontRole:
        {
            if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                {
                    ScriptCanvas::GraphVariable* graphVariable = nullptr;
                    ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                    if (graphVariable)
                    {
                        const ScriptCanvas::Data::StringType* stringValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::StringType>();

                        if (stringValue->empty())
                        {
                            QFont font;
                            font.setItalic(true);
                            return font;
                        }
                    }
                }
            }
        }
        break;
        case Qt::ToolTipRole:
        {
            ScriptCanvas::Data::Type varType;
            ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

            AZStd::string type;

            if (varType.IsValid())
            {
                type = TranslationHelper::GetSafeTypeName(varType);
            }

            if (index.column() == ColumnIndex::Type)
            {
                if (!type.empty())
                {
                    return QVariant(type.c_str());
                }
            }
            else if (index.column() == ColumnIndex::InitialValueSource)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                QString tooltipString = QString("The value of this variable can only be set within this graph");
                if (graphVariable->GetInitialValueSource() == ScriptCanvas::VariableFlags::InitialValueSource::Component)
                {
                    tooltipString = QString("The value of this variable can be set set on the component's properties");
                }

                return tooltipString;
            }
            else
            {
                AZStd::string variableName;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, varId.m_identifier);

                QString tooltipString = QString("Drag to the canvas to Get or Set %1 (Shift+Drag to Get; Alt+Drag to Set)").arg(variableName.c_str());

                if (!type.empty())
                {
                    // Prefix the type if it is valid
                    return QString("%1 - %2").arg(type.c_str(), tooltipString);
                }

                return tooltipString;
            }
           
        }
        break;
        case Qt::DecorationRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable)
                {
                    const QPixmap* icon = nullptr;

                    ScriptCanvas::Data::Type varType = graphVariable->GetDatum()->GetType();

                    if (ScriptCanvas::Data::IsContainerType(varType))
                    {
                        AZStd::vector<ScriptCanvas::Data::Type > dataTypes = ScriptCanvas::Data::GetContainedTypes(varType);

                        AZStd::vector< AZ::Uuid> azTypes;
                        azTypes.reserve(dataTypes.size());

                        for (const ScriptCanvas::Data::Type& dataType : dataTypes)
                        {
                            azTypes.push_back(ScriptCanvas::Data::ToAZType(dataType));
                        }

                        GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetMultiDataTypeIcon, azTypes);
                    }
                    else
                    {
                        GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataTypeIcon, ScriptCanvas::Data::ToAZType(varType));
                    }

                    if (icon)
                    {
                        return *icon;
                    }
                }

                return QVariant();
            }
        }
        break;

        case Qt::CheckStateRole:
        {
            if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable && varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                {
                    const ScriptCanvas::Data::BooleanType* booleanType = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::BooleanType>();

                    if ((*booleanType))
                    {
                        return Qt::CheckState::Checked;
                    }
                    else
                    {
                        return Qt::CheckState::Unchecked;
                    }
                }
            }
        }
        break;

        case Qt::TextAlignmentRole:
        {
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
        break;

        case GraphCanvas::GenericComboBoxDelegate::ComboBoxDelegateRole:
        {
            if (index.column() == ColumnIndex::Scope)
            {
                return QStringList{
                    ScriptCanvas::VariableFlags::GetScopeDisplayLabel(ScriptCanvas::VariableFlags::Scope::Graph),
                    ScriptCanvas::VariableFlags::GetScopeDisplayLabel(ScriptCanvas::VariableFlags::Scope::Function)
                };
            }
            else if (index.column() == ColumnIndex::InitialValueSource)
            {
                return QStringList{
                    tr(ScriptCanvas::GraphVariable::s_InitialValueSourceNames[0]),
                    tr(ScriptCanvas::GraphVariable::s_InitialValueSourceNames[1])
                };
            }
        }
        break;

        default:
            break;
        }

        return QVariant();
    }

    bool GraphVariablesModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        ScriptCanvas::GraphScopedVariableId varId = FindScopedVariableIdForIndex(index);
        bool modifiedData = false;

        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);

        switch (role)
        {
        case Qt::CheckStateRole:
        {
            if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                {
                    ScriptCanvas::ModifiableDatumView datumView;
                    graphVariable->ConfigureDatumView(datumView);

                    datumView.SetAs(value.toBool());

                    modifiedData = datumView.GetDatum()->GetAs<ScriptCanvas::Data::BooleanType>();
                }
            }
        }
        break;
        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                AZ::Outcome<void, AZStd::string> outcome = AZ::Failure(AZStd::string());
                AZStd::string_view oldVariableName;
                ScriptCanvas::VariableRequestBus::EventResult(oldVariableName, varId, &ScriptCanvas::VariableRequests::GetName);
                AZStd::string newVariableName = value.toString().toUtf8().data();
                if (newVariableName != oldVariableName)
                {
                    ScriptCanvas::VariableRequestBus::EventResult(outcome, varId, &ScriptCanvas::VariableRequests::RenameVariable, newVariableName);

                    modifiedData = outcome.IsSuccess();
                }
            }
            else if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable && IsEditableType(varType))
                {
                    ScriptCanvas::ModifiableDatumView datumView;
                    graphVariable->ConfigureDatumView(datumView);

                    if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                    {
                        datumView.SetAs(ScriptCanvas::Data::StringType(value.toString().toUtf8().data()));
                        modifiedData = datumView.GetDatum()->GetAs<ScriptCanvas::Data::StringType>();
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Number()))
                    {
                        datumView.SetAs(value.toDouble());
                        modifiedData = datumView.GetDatum()->GetAs<ScriptCanvas::Data::NumberType>();
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::CRC()))
                    {
                        AZStd::string newStringValue = value.toString().toUtf8().data();
                        AZ::Crc32 newCrcValue = AZ::Crc32(newStringValue.c_str());

                        const ScriptCanvas::Data::CRCType* oldCrcValue = graphVariable->GetDatum()->GetAs<ScriptCanvas::Data::CRCType>();

                        if (newCrcValue != (*oldCrcValue))
                        {
                            EditorGraphRequestBus::Event(GetScriptCanvasId(), &EditorGraphRequests::RemoveCrcCache, (*oldCrcValue));
                            EditorGraphRequestBus::Event(GetScriptCanvasId(), &EditorGraphRequests::AddCrcCache, newCrcValue, newStringValue);

                            datumView.SetAs<ScriptCanvas::Data::CRCType>(newCrcValue);
                            modifiedData = datumView.GetDatum()->GetAs<ScriptCanvas::Data::CRCType>();
                        }
                    }
                }
            }
            else if (index.column() == ColumnIndex::Scope)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable)
                {
                    QString comboBoxValue = value.toString();

                    if (!comboBoxValue.isEmpty())
                    {
                        AZStd::string scopeLabel = ScriptCanvas::VariableFlags::GetScopeDisplayLabel(graphVariable->GetScope());
                        if (scopeLabel.compare(comboBoxValue.toUtf8().data()) != 0)
                        {
                            modifiedData = true;
                            graphVariable->SetScope(ScriptCanvas::VariableFlags::GetScopeFromLabel(comboBoxValue.toUtf8().data()));
                            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::Refresh_EntireTree);
                        }
                    }
                }
            }
            else if (index.column() == ColumnIndex::InitialValueSource)
            {
                ScriptCanvas::GraphVariable* graphVariable = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

                if (graphVariable)
                {
                    QString comboBoxValue = value.toString();

                    if (!comboBoxValue.isEmpty())
                    {
                        if (graphVariable->GetInitialValueSourceName().compare(comboBoxValue.toUtf8().data()) != 0)
                        {
                            modifiedData = true;
                            graphVariable->SetInitialValueSourceFromName(comboBoxValue.toUtf8().data());
                            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::Refresh_EntireTree);
                        }
                    }
                }
            }
        }
        break;
        default:
            break;
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);

        if (modifiedData)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasId);
        }

        return modifiedData;
    }

    Qt::ItemFlags GraphVariablesModel::flags(const QModelIndex &index) const
    {
        Qt::ItemFlags itemFlags = Qt::ItemFlags(Qt::ItemIsEnabled |
                                                Qt::ItemIsDragEnabled |
                                                Qt::ItemIsSelectable);

        if (index.column() == ColumnIndex::Name)
        {
            itemFlags |= Qt::ItemIsEditable;
        }
        else if (index.column() == ColumnIndex::DefaultValue)
        {
            ScriptCanvas::GraphScopedVariableId varId = FindScopedVariableIdForIndex(index);

            ScriptCanvas::Data::Type varType;
            ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

            if (IsEditableType(varType))
            {
                if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                {
                    itemFlags |= Qt::ItemIsUserCheckable;
                }
                else
                {
                    itemFlags |= Qt::ItemIsEditable;
                }
            }
        }
        else if (index.column() == ColumnIndex::Scope)
        {
            ScriptCanvas::GraphScopedVariableId varId = FindScopedVariableIdForIndex(index);

            ScriptCanvas::GraphVariable* graphVariable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId.m_identifier);

            if (graphVariable->GetScope() != ScriptCanvas::VariableFlags::Scope::FunctionReadOnly)
            {
            itemFlags |= Qt::ItemIsEditable;
        }

        }
        else if (index.column() == ColumnIndex::InitialValueSource)
        {
            itemFlags |= Qt::ItemIsEditable;
        }

        return itemFlags;
    }

    QStringList GraphVariablesModel::mimeTypes() const
    {
        QStringList mimeTypes;
        mimeTypes.append(Widget::NodePaletteDockWidget::GetMimeType());
        return mimeTypes;
    }

    QMimeData* GraphVariablesModel::mimeData(const QModelIndexList &indexes) const
    {
        GraphCanvas::GraphCanvasMimeContainer container;

        bool isSet = ((QApplication::keyboardModifiers() & Qt::Modifier::ALT) != 0);
        bool isGet = ((QApplication::keyboardModifiers() & Qt::Modifier::SHIFT) != 0);

        ScriptCanvas::VariableId variableId;

        for (QModelIndex modelIndex : indexes)
        {
            // We select by the row, but each row still has multiple columns.
            // So to avoid handling the same row more then once, we only handle column 0.
            if (modelIndex.column() != 0)
            {
                continue;
            }

            variableId = FindVariableIdForIndex(modelIndex);

            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = nullptr;

            if (isSet)
            {
                mimeEvent = aznew CreateSetVariableNodeMimeEvent(variableId);
            }
            else if (isGet)
            {
                mimeEvent = aznew CreateGetVariableNodeMimeEvent(variableId);
            }
            else
            {
                mimeEvent = aznew CreateVariableSpecificNodeMimeEvent(variableId);
            }

            if (mimeEvent)
            {
                container.m_mimeEvents.push_back(mimeEvent);
            }
        }

        if (container.m_mimeEvents.empty())
        {
            return nullptr;
        }

        AZStd::vector<char> encoded;
        if (!container.ToBuffer(encoded))
        {
            return nullptr;
        }

        QMimeData* mimeDataPtr = new QMimeData();

        {
            QByteArray encodedData;
            encodedData.resize((int)encoded.size());
            memcpy(encodedData.data(), encoded.data(), encoded.size());
            mimeDataPtr->setData(Widget::NodePaletteDockWidget::GetMimeType(), encodedData);
        }

        encoded.clear();

        if (container.m_mimeEvents.size() == 1)
        {
            GraphCanvas::QtMimeUtils::WriteTypeToMimeData<ScriptCanvas::VariableId>(mimeDataPtr, GraphCanvas::k_ReferenceMimeType, variableId);
        }

        return mimeDataPtr;
    }
    
    void GraphVariablesModel::SetActiveScene(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
        m_assetType = azrtti_typeid<ScriptCanvasAsset>();
        m_scriptCanvasId = scriptCanvasId;

        if (m_scriptCanvasId.IsValid())
        {
            ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusConnect(m_scriptCanvasId);
        }

        PopulateSceneVariables();
    }

    ScriptCanvas::ScriptCanvasId GraphVariablesModel::GetScriptCanvasId() const
    {
        return m_scriptCanvasId;
    }

    bool GraphVariablesModel::IsEditableType(ScriptCanvas::Data::Type scriptCanvasDataType) const
    {
        return scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::String())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::Number())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::Boolean())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::CRC());
    }

    void GraphVariablesModel::PopulateSceneVariables()
    {
        layoutAboutToBeChanged();

        ScriptCanvas::VariableNotificationBus::MultiHandler::BusDisconnect();
        m_variableIds.clear();
    
        const AZStd::unordered_map<ScriptCanvas::VariableId, ScriptCanvas::GraphVariable>* variableMap = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableMap, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariables);

        if (variableMap)
        {
            m_variableIds.reserve(variableMap->size());

            for (const AZStd::pair<ScriptCanvas::VariableId, ScriptCanvas::GraphVariable>& element : *variableMap)
            {
                ScriptCanvas::GraphScopedVariableId notificationId = element.second.GetGraphScopedId();

                ScriptCanvas::VariableNotificationBus::MultiHandler::BusConnect(notificationId);
                m_variableIds.push_back(notificationId);
            }
        }

        layoutChanged();
    }

    void GraphVariablesModel::OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        ScriptCanvas::GraphVariable* graphVariable = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

        ScriptCanvas::GraphScopedVariableId notificationId = graphVariable->GetGraphScopedId();

        int index = static_cast<int>(m_variableIds.size());

        beginInsertRows(QModelIndex(), index, index);
        m_variableIds.push_back(notificationId);
        endInsertRows();        

        ScriptCanvas::VariableNotificationBus::MultiHandler::BusConnect(notificationId);

        QModelIndex modelIndex = createIndex(index, 0);
        emit VariableAdded(modelIndex);
    }

    void GraphVariablesModel::OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int index = FindRowForVariableId(variableId);

        if (index >= 0)
        {
            ScriptCanvas::GraphScopedVariableId notificationId = m_variableIds[index];

            beginRemoveRows(QModelIndex(), index, index);
            m_variableIds.erase(m_variableIds.begin() + index);
            endRemoveRows();

            ScriptCanvas::VariableNotificationBus::MultiHandler::BusDisconnect(notificationId);
        }
    }

    void GraphVariablesModel::OnVariableNameChangedInGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int index = FindRowForVariableId(variableId);

        if (index >= 0)
        {
            QModelIndex modelIndex = createIndex(index, ColumnIndex::Name, nullptr);
            dataChanged(modelIndex, modelIndex);

        }
    }

    void GraphVariablesModel::OnVariableValueChanged()
    {
        const ScriptCanvas::GraphScopedVariableId* variableId = ScriptCanvas::VariableNotificationBus::GetCurrentBusId();

        int index = FindRowForVariableId((*variableId).m_identifier);

        if (index >= 0)
        {
            QModelIndex modelIndex = createIndex(index, ColumnIndex::DefaultValue, nullptr);
            dataChanged(modelIndex, modelIndex);
        }
    }

    void GraphVariablesModel::OnVariableScopeChanged()
    {
        const ScriptCanvas::GraphScopedVariableId* variableId = ScriptCanvas::VariableNotificationBus::GetCurrentBusId();

        int index = FindRowForVariableId((*variableId).m_identifier);

        if (index >= 0)
        {
            QModelIndex modelIndex = createIndex(index, ColumnIndex::Scope, nullptr);
            dataChanged(modelIndex, modelIndex);
        }
    }

    void GraphVariablesModel::OnVariableInitialValueSourceChanged()
    {
        const ScriptCanvas::GraphScopedVariableId* variableId = ScriptCanvas::VariableNotificationBus::GetCurrentBusId();

        int index = FindRowForVariableId((*variableId).m_identifier);

        if (index >= 0)
        {
            QModelIndex modelIndex = createIndex(index, ColumnIndex::InitialValueSource, nullptr);
            dataChanged(modelIndex, modelIndex);
        }
    }

    void GraphVariablesModel::OnVariablePriorityChanged()
    {
        const ScriptCanvas::GraphScopedVariableId* variableId = ScriptCanvas::VariableNotificationBus::GetCurrentBusId();

        int index = FindRowForVariableId((*variableId).m_identifier);

        if (index >= 0)
        {
            QModelIndex modelIndex = createIndex(index, 0, nullptr);
            QModelIndex otherIndex = createIndex(index, ColumnIndex::Count - 1, nullptr);

            dataChanged(modelIndex, otherIndex);
        }
    }

    QVariant GraphVariablesModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            return tr(m_columnNames[section]);
        }

        if (role == Qt::TextAlignmentRole)
        {
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    ScriptCanvas::VariableId GraphVariablesModel::FindVariableIdForIndex(const QModelIndex& index) const
    {
        ScriptCanvas::VariableId variableId;

        if (index.row() >= 0 && index.row() < m_variableIds.size())
        {
            variableId = m_variableIds[index.row()].m_identifier;
        }

        return variableId;
    }

    ScriptCanvas::GraphScopedVariableId GraphVariablesModel::FindScopedVariableIdForIndex(const QModelIndex& index) const
    {
        ScriptCanvas::GraphScopedVariableId scopedVariableId;

        if (index.row() >= 0 && index.row() < m_variableIds.size())
        {
            scopedVariableId = m_variableIds[index.row()];
        }

        return scopedVariableId;
    }

    int GraphVariablesModel::FindRowForVariableId(const ScriptCanvas::VariableId& variableId) const
    {
        for (int i = 0; i < static_cast<int>(m_variableIds.size()); ++i)
        {
            if (m_variableIds[i].m_identifier == variableId)
            {
                return i;
            }
        }

        return -1;
    }

    bool GraphVariablesModel::IsFunction()const
    {
        return m_assetType == azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>();
    }

    ////////////////////////////////////////////
    // GraphVariablesModelSortFilterProxyModel
    ////////////////////////////////////////////

    GraphVariablesModelSortFilterProxyModel::GraphVariablesModelSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {

    }

    bool GraphVariablesModelSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }

        GraphVariablesModel* model = qobject_cast<GraphVariablesModel*>(sourceModel());
        if (!model)
        {
            return false;
        }

        QModelIndex index = model->index(sourceRow, GraphVariablesModel::ColumnIndex::Name, sourceParent);
        QString test = model->data(index, Qt::DisplayRole).toString();

        return (test.lastIndexOf(m_filterRegex) >= 0);
    }

    bool GraphVariablesModelSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        GraphVariablesModel* model = qobject_cast<GraphVariablesModel*>(sourceModel());
        if (!model)
        {
            return false;
        }

        ScriptCanvas::VariableId leftVariableId = model->FindVariableIdForIndex(left);
        ScriptCanvas::VariableId rightVariableId = model->FindVariableIdForIndex(right);

        ScriptCanvas::GraphVariableManagerRequests* requests = ScriptCanvas::GraphVariableManagerRequestBus::FindFirstHandler(model->GetScriptCanvasId());

        ScriptCanvas::GraphVariable* leftVariable = requests->FindVariableById(leftVariableId);
        ScriptCanvas::GraphVariable* rightVariable = requests->FindVariableById(rightVariableId);

        if (leftVariable == nullptr)
        {
            return true;
        }

        if (rightVariable == nullptr)
        { 
            return false;
        }

        int leftPriority = leftVariable->GetSortPriority();
        int rightPriority = rightVariable->GetSortPriority();

        if (leftPriority == rightPriority)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }
        else
        {
            return m_variableComparator(leftVariable, rightVariable);
        }
    }

    void GraphVariablesModelSortFilterProxyModel::SetFilter(const QString& filter)
    {
        m_filter = QRegExp::escape(filter);
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }

    ////////////////////////////
    // GraphVariablesTableView
    ////////////////////////////

    bool GraphVariablesTableView::HasCopyVariableData()
    {
        return QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasFormat(ScriptCanvas::CopiedVariableData::k_variableKey);
    }

    void GraphVariablesTableView::CopyVariableToClipboard(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const ScriptCanvas::VariableId& variableId)
    {
        ScriptCanvas::GraphVariable* graphVariable = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

        if (graphVariable == nullptr)
        {
            return;
        }

        ScriptCanvas::CopiedVariableData copiedVariableData;
        copiedVariableData.m_variableMapping[variableId] = (*graphVariable);

        AZStd::vector<char> buffer;
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();        

        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
        AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &copiedVariableData, serializeContext);

        QMimeData* mime = new QMimeData();
        mime->setData(ScriptCanvas::CopiedVariableData::k_variableKey, QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));

        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setMimeData(mime);
    }

    bool GraphVariablesTableView::HandleVariablePaste(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        QClipboard* clipboard = QApplication::clipboard();

        // Trying to paste unknown data into our scene.
        if (!HasCopyVariableData())
        {
            return false;
        }

        ScriptCanvas::CopiedVariableData copiedVariableData;

        QByteArray byteArray = clipboard->mimeData()->data(ScriptCanvas::CopiedVariableData::k_variableKey);

        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        AZ::Utils::LoadObjectFromBufferInPlace(byteArray.constData(), static_cast<AZStd::size_t>(byteArray.size()), copiedVariableData, serializeContext);

        ScriptCanvas::GraphVariableManagerRequests* requests = ScriptCanvas::GraphVariableManagerRequestBus::FindFirstHandler(scriptCanvasId);

        if (requests == nullptr)
        {
            return false;
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);

        for (auto variableMapData : copiedVariableData.m_variableMapping)
        {
            requests->CloneVariable(variableMapData.second);
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasId);

        return !copiedVariableData.m_variableMapping.empty();
    }

    GraphVariablesTableView::GraphVariablesTableView(QWidget* parent)
        : QTableView(parent)
        , m_nextInstanceAction(nullptr)
        , m_previousInstanceAction(nullptr)
    {
        m_model = aznew GraphVariablesModel(this);
        m_proxyModel = aznew GraphVariablesModelSortFilterProxyModel(this);

        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        setModel(m_proxyModel);

        ApplyPreferenceSort();
        setItemDelegateForColumn(GraphVariablesModel::Name, aznew GraphCanvas::IconDecoratedNameDelegate(this));
        setItemDelegateForColumn(GraphVariablesModel::Scope, aznew GraphCanvas::GenericComboBoxDelegate(this));
        setItemDelegateForColumn(GraphVariablesModel::InitialValueSource, aznew GraphCanvas::GenericComboBoxDelegate(this));

        horizontalHeader()->setStretchLastSection(false);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::Name, QHeaderView::ResizeMode::ResizeToContents);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::DefaultValue, QHeaderView::ResizeMode::ResizeToContents);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::Type, QHeaderView::ResizeMode::Stretch);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::Scope, QHeaderView::ResizeMode::Stretch);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::InitialValueSource, QHeaderView::ResizeMode::Stretch);
        horizontalHeader()->show();

        {
            QAction* deleteAction = new QAction(this);
            deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));

            QObject::connect(deleteAction, &QAction::triggered, this, &GraphVariablesTableView::OnDeleteSelected);

            addAction(deleteAction);
        }

        {
            QAction* copyAction = new QAction(this);
            copyAction->setShortcut(QKeySequence::Copy);

            QObject::connect(copyAction, &QAction::triggered, this, &GraphVariablesTableView::OnCopySelected);

            addAction(copyAction);
        }

        {
            QAction* pasteAction = new QAction(this);
            pasteAction->setShortcut(QKeySequence::Paste);

            QObject::connect(pasteAction, &QAction::triggered, this, &GraphVariablesTableView::OnPaste);

            addAction(pasteAction);
        }

        {
            QAction* duplicateAction = new QAction(this);
            duplicateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));

            QObject::connect(duplicateAction, &QAction::triggered, this, &GraphVariablesTableView::OnDuplicate);

            addAction(duplicateAction);
        }

        {
            m_nextInstanceAction = new QAction(this);
            m_nextInstanceAction->setShortcut(QKeySequence(Qt::Key_F8));

            QObject::connect(m_nextInstanceAction, &QAction::triggered, this, &GraphVariablesTableView::CycleToNextVariableReference);

            addAction(m_nextInstanceAction);
        }

        {
            m_previousInstanceAction = new QAction(this);
            m_previousInstanceAction->setShortcut(QKeySequence(Qt::Key_F7));

            QObject::connect(m_previousInstanceAction, &QAction::triggered, this, &GraphVariablesTableView::CycleToPreviousVariableReference);

            addAction(m_previousInstanceAction);
        }

        QObject::connect(m_model, &GraphVariablesModel::VariableAdded, this, &GraphVariablesTableView::OnVariableAdded);

        setMinimumSize(0, 0);
        ResizeColumns();
    }

    void GraphVariablesTableView::SetActiveScene(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        clearSelection();
        m_model->SetActiveScene(scriptCanvasId);

        m_scriptCanvasId = scriptCanvasId;

        m_graphCanvasGraphId.SetInvalid();
        GeneralRequestBus::BroadcastResult(m_graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, m_scriptCanvasId);

        m_cyclingHelper.SetActiveGraph(m_graphCanvasGraphId);

        ResizeColumns();
    }

    void GraphVariablesTableView::SetFilter(const QString& filterString)
    {
        clearSelection();
        m_proxyModel->SetFilter(filterString);
    }

    void GraphVariablesTableView::EditVariableName(ScriptCanvas::VariableId variableId)
    {
        int row = m_model->FindRowForVariableId(variableId);

        QModelIndex sourceIndex = m_model->index(row, GraphVariablesModel::ColumnIndex::Name);
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);

        setCurrentIndex(proxyIndex);
        edit(proxyIndex);
    }

    void GraphVariablesTableView::hideEvent(QHideEvent* event)
    {
        QTableView::hideEvent(event);

        clearSelection();        
        m_proxyModel->SetFilter("");
    }

    void GraphVariablesTableView::resizeEvent(QResizeEvent* resizeEvent)
    {
        QTableView::resizeEvent(resizeEvent);

        ResizeColumns();
    }

    void GraphVariablesTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QTableView::selectionChanged(selected, deselected);

        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        AZStd::unordered_set< ScriptCanvas::VariableId > variableSet;

        QModelIndexList indexList = selectedIndexes();

        for (const QModelIndex& index : indexList)
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

            variableSet.insert(m_model->FindVariableIdForIndex(sourceIndex));
        }

        if (variableSet.size() == 1)
        {
            SetCycleTarget((*variableSet.begin()));
        }
        else
        {
            SetCycleTarget(ScriptCanvas::VariableId());
        }

        emit SelectionChanged(variableSet);

        if (!selected.isEmpty())
        {
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphCanvasGraphId);
        }
    }

    void GraphVariablesTableView::OnSelectionChanged()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        clearSelection();
    }

    void GraphVariablesTableView::ApplyPreferenceSort()
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        m_proxyModel->sort(settings->m_variablePanelSorting);
    }

    void GraphVariablesTableView::OnVariableAdded(QModelIndex modelIndex)
    {
        bool isUndo = false;
        GeneralRequestBus::BroadcastResult(isUndo, &GeneralRequests::IsActiveGraphInUndoRedo);

        if (!isUndo)
        {
            clearSelection();
            m_proxyModel->SetFilter("");

            QModelIndex proxyIndex = m_proxyModel->mapFromSource(modelIndex);

            scrollTo(proxyIndex);
            selectionModel()->select(QItemSelection(proxyIndex, proxyIndex), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        ResizeColumns();
    }

    void GraphVariablesTableView::OnDeleteSelected()
    {
        AZStd::unordered_set< ScriptCanvas::VariableId > variableSet;

        QModelIndexList indexList = selectedIndexes();

        for (const QModelIndex& index : indexList)
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

            variableSet.insert(m_model->FindVariableIdForIndex(sourceIndex));
        }

        emit DeleteVariables(variableSet);

        ResizeColumns();
    }

    void GraphVariablesTableView::ResizeColumns()
    {
        int availableWidth = width();

        int typeLength = aznumeric_cast<int>(availableWidth * 0.3f);

        int maxTypeLength = sizeHintForColumn(GraphVariablesModel::Type) + 10;

        if (typeLength >= maxTypeLength)
        {
            typeLength = maxTypeLength;
        }

        int defaultValueLength = aznumeric_cast<int>(availableWidth * 0.2f);

        horizontalHeader()->resizeSection(GraphVariablesModel::Type, typeLength);
        horizontalHeader()->resizeSection(GraphVariablesModel::DefaultValue, defaultValueLength);

        horizontalHeader()->resizeSection(GraphVariablesModel::Scope, 100);

        int remainingLength = aznumeric_cast<int>(availableWidth * 0.1f);
        int maxExposureLength = 80;

        if (remainingLength >= maxExposureLength)
        {
            remainingLength = maxExposureLength;
        }

        horizontalHeader()->resizeSection(GraphVariablesModel::InitialValueSource, 120);
    }

    void GraphVariablesTableView::OnCopySelected()
    {
        QModelIndexList indexList = selectedIndexes();

        if (!indexList.empty())
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(indexList.front());

            ScriptCanvas::VariableId variableId = m_model->FindVariableIdForIndex(sourceIndex);
            CopyVariableToClipboard(m_scriptCanvasId, variableId);
        }
    }

    void GraphVariablesTableView::OnPaste()
    {
        HandleVariablePaste(m_scriptCanvasId);
    }

    void GraphVariablesTableView::SetCycleTarget(ScriptCanvas::VariableId variableId)
    {
        m_cyclingHelper.Clear();
        m_cyclingVariableId = variableId;

        if (m_nextInstanceAction)
        {
            m_nextInstanceAction->setEnabled(m_cyclingVariableId.IsValid());
            m_previousInstanceAction->setEnabled(m_cyclingVariableId.IsValid());
        }
    }

    void GraphVariablesTableView::OnDuplicate()
    {
        QModelIndexList indexList = selectedIndexes();

        if (!indexList.empty())
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(indexList.front());

            ScriptCanvas::VariableId variableId = m_model->FindVariableIdForIndex(sourceIndex);

            ScriptCanvas::GraphVariable* graphVariable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

            if (graphVariable == nullptr)
            {
                return;
            }

            GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
            ScriptCanvas::GraphVariableManagerRequestBus::Event(m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::CloneVariable, (*graphVariable));
            GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasId);
        }
    }

    void GraphVariablesTableView::CycleToNextVariableReference()
    {
        ConfigureHelper();

        m_cyclingHelper.CycleToNextNode();
    }

    void GraphVariablesTableView::CycleToPreviousVariableReference()
    {
        ConfigureHelper();

        m_cyclingHelper.CycleToPreviousNode();
    }

    void GraphVariablesTableView::ConfigureHelper()
    {
        if (!m_cyclingHelper.IsConfigured() && m_cyclingVariableId.IsValid())
        {
            AZStd::vector< NodeIdPair > nodeIds;
            EditorGraphRequestBus::EventResult(nodeIds, m_scriptCanvasId, &EditorGraphRequests::GetVariableNodes, m_cyclingVariableId);

            AZStd::vector< GraphCanvas::NodeId > canvasNodes;
            canvasNodes.reserve(nodeIds.size());

            for (const auto& nodeIdPair : nodeIds)
            {
                canvasNodes.emplace_back(nodeIdPair.m_graphCanvasId);
            }

            m_cyclingHelper.SetNodes(canvasNodes);
        }
    }

#include <Editor/View/Widgets/VariablePanel/moc_GraphVariablesTableView.cpp>
}
