/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DocumentPropertyEditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>

#include <AzCore/DOM/DomUtils.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AzToolsFramework
{
    // helper method used by both the view and row wdigets
    static void destroyLayoutContents(QLayout* layout)
    {
        while (auto layoutItem = layout->takeAt(0))
        {
            auto subWidget = layoutItem->widget();
            if (subWidget)
            {
                delete subWidget;
            }
            delete layoutItem;
        }
    }

    DPERowWidget::DPERowWidget(QWidget* parentWidget)
        : QWidget(parentWidget)
    {
        m_mainLayout = new QVBoxLayout(this);
        setLayout(m_mainLayout);
        m_columnLayout = new QHBoxLayout();
        m_mainLayout->addLayout(m_columnLayout);
        m_childLayout = new QVBoxLayout();
        m_mainLayout->addLayout(m_childLayout);
    }

    DPERowWidget::~DPERowWidget()
    {
        Clear();
    }

    void DPERowWidget::Clear()
    {
        destroyLayoutContents(m_columnLayout);
        destroyLayoutContents(m_childLayout);
    }

    void DPERowWidget::PopulateFromValue(const AZ::Dom::Value& domValue)
    {
        Clear();

        // populate all direct children of this row
        for (size_t arrayIndex = 0, numIndices = domValue.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& childValue = domValue[arrayIndex];
            auto childType = childValue.GetNodeName().GetStringView();

            if (childType == AZ::DocumentPropertyEditor::Nodes::Row::Name)
            {
                auto newRow = new DPERowWidget(this);
                newRow->PopulateFromValue(childValue);
                m_childLayout->addWidget(newRow);
            }
            else if (childType == AZ::DocumentPropertyEditor::Nodes::Label::Name)
            {
                auto labelString = childValue[AZ::DocumentPropertyEditor::Nodes::Label::Value.GetName()].GetString();
                m_columnLayout->addWidget(new QLabel(QString::fromUtf8(labelString.data(), static_cast<int>(labelString.size())), this));
            }
            else if (childType == AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Name)
            {
                // <apm> temporary until handler widgets are done
                AZ::Dom::JsonBackend<AZ::Dom::Json::ParseFlags::ParseComments, AZ::Dom::Json::OutputFormatting::MinifiedJson> jsonBackend;
                AZStd::string stringBuffer;
                AZ::Dom::Utils::ValueToSerializedString(
                    jsonBackend, childValue[AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.GetName()], stringBuffer);
                m_columnLayout->addWidget(
                    new QLineEdit(QString::fromUtf8(stringBuffer.data(), static_cast<int>(stringBuffer.size())), this));
            }
        }
    }

    DocumentPropertyEditor::DocumentPropertyEditor(QWidget* parentWidget)
        : QFrame(parentWidget)
    {
        new QVBoxLayout(this);
    }

    DocumentPropertyEditor::~DocumentPropertyEditor()
    {
        destroyLayoutContents(GetVerticalLayout());
    }

    void DocumentPropertyEditor::SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter)
    {
        m_adapter = theAdapter;
        m_resetHandler = AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler(
            [this]()
            {
                this->HandleReset();
            });
        m_adapter->ConnectResetHandler(m_resetHandler);

        m_changedHandler = AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler(
            [this](const AZ::Dom::Patch& patch)
            {
                this->HandleDomChange(patch);
            });
        m_adapter->ConnectChangedHandler(m_changedHandler);

        // populate the view, just like a reset
        HandleReset();
    }

    QVBoxLayout* DocumentPropertyEditor::GetVerticalLayout()
    {
        return static_cast<QVBoxLayout*>(layout());
    }

    void DocumentPropertyEditor::HandleReset()
    {
        QVBoxLayout* mainLayout = GetVerticalLayout();
        destroyLayoutContents(mainLayout);

        auto topContents = m_adapter->GetContents();

        for (size_t arrayIndex = 0, numIndices = topContents.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& rowValue = topContents[arrayIndex];
            auto domName = rowValue.GetNodeName().GetStringView();
            const bool isRow = (domName == AZ::DocumentPropertyEditor::Nodes::Row::Name);
            AZ_Assert(isRow, "adapters must only have rows as direct children!");

            if (isRow)
            {
                auto newRow = new DPERowWidget(this);
                newRow->PopulateFromValue(rowValue);
                mainLayout->addWidget(newRow);
            }
        }
    }
    void DocumentPropertyEditor::HandleDomChange([[maybe_unused]] const AZ::Dom::Patch& patch)
    {
        // <apm> implement
    }

} // namespace AzToolsFramework