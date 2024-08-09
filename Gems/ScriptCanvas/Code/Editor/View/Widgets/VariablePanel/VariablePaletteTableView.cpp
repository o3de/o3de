/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qaction.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qitemselectionmodel.h>
#include <qscrollbar.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariablePaletteTableView.h>
#include <Editor/View/Widgets/DataTypePalette/DataTypePaletteModel.h>

#include <Editor/View/Dialogs/ContainerWizard/ContainerWizard.h>

#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/QtMetaTypes.h>

#include <ScriptCanvas/Data/DataRegistry.h>

namespace ScriptCanvasEditor
{    
    /////////////////////////////
    // VariablePaletteTableView
    /////////////////////////////
    
    VariablePaletteTableView::VariablePaletteTableView(QWidget* parent)
        : QTableView(parent)
    {
        m_containerWizard = aznew ContainerWizard(this);

        m_model = aznew DataTypePaletteModel(this);
        m_proxyModel = aznew DataTypePaletteSortFilterProxyModel(this);
        
        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->sort(DataTypePaletteModel::ColumnIndex::Type);

        setModel(m_proxyModel);

        setItemDelegateForColumn(DataTypePaletteModel::Type, aznew GraphCanvas::IconDecoratedNameDelegate(this));

        viewport()->installEventFilter(this);
        horizontalHeader()->setSectionResizeMode(DataTypePaletteModel::ColumnIndex::Pinned, QHeaderView::ResizeMode::ResizeToContents);
        horizontalHeader()->setSectionResizeMode(DataTypePaletteModel::ColumnIndex::Type, QHeaderView::ResizeMode::Stretch);

        QObject::connect(this, &QAbstractItemView::clicked, this, &VariablePaletteTableView::OnClicked);

        QObject::connect(m_containerWizard, &ContainerWizard::CreateContainerVariable, this, &VariablePaletteTableView::OnCreateContainerVariable);
        QObject::connect(m_containerWizard, &ContainerWizard::ContainerPinned, this, &VariablePaletteTableView::OnContainerPinned);

        m_completer = new QCompleter();

        m_completer->setModel(m_proxyModel);
        m_completer->setCompletionColumn(DataTypePaletteModel::ColumnIndex::Type);
        m_completer->setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
        m_completer->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

        setMinimumSize(0,0);
    }

    VariablePaletteTableView::~VariablePaletteTableView()
    {
        m_model->SubmitPendingPinChanges();
    }

    void VariablePaletteTableView::SetActiveScene(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        m_containerWizard->SetActiveScriptCanvasId(scriptCanvasId);
    }

    void VariablePaletteTableView::PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes)
    {
        clearSelection();
        m_model->ClearTypes();

        AZStd::unordered_set<AZ::Uuid> variableTypes;

        auto dataRegistry = ScriptCanvas::GetDataRegistry();        

        for (const auto& dataTraitsPair : dataRegistry->m_typeIdTraitMap)
        {
            // Object type isn't valid on it's own. Need to skip that in order. Passed in types will all be
            // processed as an object type.
            if (dataTraitsPair.first == ScriptCanvas::Data::eType::BehaviorContextObject)
            {
                continue;
            }

            AZ::Uuid typeId = dataTraitsPair.second.m_dataTraits.GetAZType();

            if (typeId.IsNull() || typeId == azrtti_typeid<void>())
            {
                continue;
            }

            m_containerWizard->RegisterType(typeId);
            variableTypes.insert(typeId);
        }

        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC_CE("ScriptCanvasPreviewSettings"), AZ::UserSettings::CT_LOCAL);

        for (const AZ::Uuid& objectId : objectTypes)
        {
            ScriptCanvas::Data::Type type = dataRegistry->m_typeIdTraitMap[ScriptCanvas::Data::eType::BehaviorContextObject].m_dataTraits.GetSCType(objectId);
            if (!type.IsValid() || !dataRegistry->m_creatableTypes.contains(type))
            {
                continue;
            }

            // For now, we need to register all of the objectId's with the container wizard
            // in order to properly populate the list of valid container configurations.
            m_containerWizard->RegisterType(objectId);

            // Sanitize containers so we only put in information about the generic container type
            if (AZ::Utils::IsContainerType(objectId))
            {
                variableTypes.insert(AZ::Utils::GetGenericContainerType(objectId));
            }
            else
            {
                variableTypes.insert(objectId);
            }
        }

        // Since we gated containers to make them genrealized buckets, we now need to go through
        // and register in the custom defined types for each of the container types that we created.
        for (const AZ::Uuid& pinnedTypeId : settings->m_pinnedDataTypes)
        {
            if (variableTypes.find(pinnedTypeId) == variableTypes.end())
            {
                variableTypes.insert(pinnedTypeId);
            }
        }

        m_model->PopulateVariablePalette(variableTypes);
    }

    void VariablePaletteTableView::SetFilter(const QString& filter)
    {
        m_model->SubmitPendingPinChanges();

        clearSelection();
        m_proxyModel->SetFilter(filter);
    }

    QCompleter* VariablePaletteTableView::GetVariableCompleter()
    {
        return m_completer;
    }

    void VariablePaletteTableView::TryCreateVariableByTypeName(const AZStd::string& typeName)
    {        
        AZ::TypeId typeId = m_model->FindTypeIdForTypeName(typeName);

        // Only want to go into the wizard, if we have a generic type.
        if (AZ::Utils::IsGenericContainerType(typeId))
        {
            m_containerWizard->ShowWizard(typeId);
        }
        else if (!typeId.IsNull() && typeId != azrtti_typeid<void>())
        {
            emit CreateVariable(ScriptCanvas::Data::FromAZType(typeId));
        }
    }

    void VariablePaletteTableView::hideEvent(QHideEvent* hideEvent)
    {
        m_model->SubmitPendingPinChanges();
        clearSelection();

        QTableView::hideEvent(hideEvent);
    }

    void VariablePaletteTableView::showEvent(QShowEvent* showEvent)
    {
        QTableView::showEvent(showEvent);

        clearSelection();
        scrollToTop();

        m_proxyModel->invalidate();
    }

    const DataTypePaletteModel* VariablePaletteTableView::GetVariableTypePaletteModel() const
    {
        return m_model;
    }

    AZStd::vector< AZ::TypeId > VariablePaletteTableView::GetArrayTypes() const
    {
        AZStd::vector< AZ::TypeId > arrayTypes;

        const auto& finalTypeMapping = m_containerWizard->GetFinalTypeMapping();

        for (auto typePair : finalTypeMapping)
        {            
            if (ScriptCanvas::Data::IsVectorContainerType(ScriptCanvas::Data::FromAZType(typePair.second)))
            {
                arrayTypes.emplace_back(typePair.second);
            }
        }

        return arrayTypes;
    }

    AZStd::vector< AZ::TypeId > VariablePaletteTableView::GetMapTypes() const
    {
        AZStd::vector< AZ::TypeId > mapTypes;

        const auto& finalTypeMapping = m_containerWizard->GetFinalTypeMapping();

        for (auto typePair : finalTypeMapping)
        {
            if (ScriptCanvas::Data::IsMapContainerType(ScriptCanvas::Data::FromAZType(typePair.second)))
            {
                mapTypes.emplace_back(typePair.second);
            }
        }

        return mapTypes;
    }

    void VariablePaletteTableView::OnClicked(const QModelIndex& index)
    {
        AZ::TypeId typeId;

        QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

        if (sourceIndex.isValid())
        {
            typeId = m_model->FindTypeIdForIndex(sourceIndex);
        }

        if (!typeId.IsNull() && typeId != azrtti_typeid<void>())
        {
            if (index.column() == DataTypePaletteModel::ColumnIndex::Pinned)
            {
                m_model->TogglePendingPinChange(typeId);
                m_model->dataChanged(sourceIndex, sourceIndex);
            }
            else if (AZ::Utils::IsGenericContainerType(typeId))
            {
                m_containerWizard->ShowWizard(typeId);
            }
            else
            {
                emit CreateVariable(ScriptCanvas::Data::FromAZType(typeId));
            }
        }
    }

    void VariablePaletteTableView::OnContainerPinned(const AZ::TypeId& typeId)
    {
        m_model->AddDataType(typeId);
    }

    void VariablePaletteTableView::OnCreateContainerVariable(const AZStd::string& variableName, const AZ::TypeId& typeId)
    {
        ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::FromAZType(typeId);
        emit CreateNamedVariable(variableName, dataType);
    }

#include <Editor/View/Widgets/VariablePanel/moc_VariablePaletteTableView.cpp>
}
