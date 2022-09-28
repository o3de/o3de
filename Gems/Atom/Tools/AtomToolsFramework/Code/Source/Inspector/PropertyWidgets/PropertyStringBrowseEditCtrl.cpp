/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    PropertyStringBrowseEditCtrl::PropertyStringBrowseEditCtrl(QWidget* parent)
        : QWidget(parent)
    {
        setLayout(new QHBoxLayout(this));

        m_browseEdit = new AzQtComponents::BrowseEdit(this);
        m_browseEdit->lineEdit()->setFocusPolicy(Qt::StrongFocus);
        m_browseEdit->setLineEditReadOnly(false);
        m_browseEdit->setClearButtonEnabled(true);
        m_browseEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_browseEdit->setAttachedButtonIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));
        QObject::connect(m_browseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &PropertyStringBrowseEditCtrl::Edit);
        QObject::connect(m_browseEdit, &AzQtComponents::BrowseEdit::editingFinished, this, &PropertyStringBrowseEditCtrl::ValueChanged);

        SetEditButtonVisible(false);
        SetEditButtonEnabled(false);

        // Locate the clear button embedded inside the browse edit control to connect to its clicked signal
        if (auto clearButton = AzQtComponents::LineEdit::getClearButton(m_browseEdit->lineEdit()))
        {
            clearButton->setVisible(true);
            clearButton->setEnabled(true);
            QObject::connect(clearButton, &QToolButton::clicked, this, &PropertyStringBrowseEditCtrl::Clear);
        }

        layout()->setContentsMargins(0, 0, 0, 0);
        layout()->addWidget(m_browseEdit);

        setFocusProxy(m_browseEdit->lineEdit());
        setFocusPolicy(m_browseEdit->lineEdit()->focusPolicy());
    }

    void PropertyStringBrowseEditCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue)
    {
        if (attrib == AZ_CRC_CE("LineEditReadOnly"))
        {
            if (bool value = true; attrValue->Read<bool>(value))
            {
                m_browseEdit->setLineEditReadOnly(value);
            }
            return;
        }

        if (attrib == AZ_CRC_CE("ClearButtonEnabled"))
        {
            if (bool value = true; attrValue->Read<bool>(value))
            {
                m_browseEdit->setClearButtonEnabled(value);
            }
            return;
        }

        if (attrib == AZ_CRC_CE("EditButtonIcon"))
        {
            if (AZStd::string value; attrValue->Read<AZStd::string>(value))
            {
                m_browseEdit->setAttachedButtonIcon(QIcon(value.c_str()));
            }
            return;
        }

        if (attrib == AZ_CRC_CE("EditButtonVisible"))
        {
            if (bool value = true; attrValue->Read<bool>(value))
            {
                SetEditButtonVisible(value);
            }
            return;
        }

        if (attrib == AZ_CRC_CE("EditButtonEnabled"))
        {
            if (bool value = true; attrValue->Read<bool>(value))
            {
                SetEditButtonEnabled(value);
            }
            return;
        }
    }

    void PropertyStringBrowseEditCtrl::Edit()
    {
        Q_EMIT ValueChanged();
    }

    void PropertyStringBrowseEditCtrl::Clear()
    {
        m_browseEdit->setText("");
        m_browseEdit->lineEdit()->clearFocus();
        Q_EMIT ValueChanged();
    }

    void PropertyStringBrowseEditCtrl::SetValue(const AZStd::string& value)
    {
        m_browseEdit->setText(value.c_str());
        Q_EMIT ValueChanged();
    }

    AZStd::string PropertyStringBrowseEditCtrl::GetValue() const
    {
        return m_browseEdit->text().toUtf8().constData();
    }

    void PropertyStringBrowseEditCtrl::SetEditButtonEnabled(bool value)
    {
        if (auto editButton = m_browseEdit->findChild<QPushButton*>())
        {
            editButton->setEnabled(value);
        }
    }

    void PropertyStringBrowseEditCtrl::SetEditButtonVisible(bool value)
    {
        if (auto editButton = m_browseEdit->findChild<QPushButton*>())
        {
            editButton->setVisible(value);
        }
    }

    QWidget* PropertyStringBrowseEditHandler::CreateGUI(QWidget* parent)
    {
        PropertyStringBrowseEditCtrl* newCtrl = aznew PropertyStringBrowseEditCtrl(parent);

        QObject::connect(
            newCtrl,
            &PropertyStringBrowseEditCtrl::ValueChanged,
            this,
            [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });

        return newCtrl;
    }

    void PropertyStringBrowseEditHandler::ConsumeAttribute(
        [[maybe_unused]] PropertyStringBrowseEditCtrl* GUI,
        [[maybe_unused]] AZ::u32 attrib,
        [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue,
        [[maybe_unused]] const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue);
    }

    void PropertyStringBrowseEditHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyStringBrowseEditCtrl* GUI,
        [[maybe_unused]] property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue();
    }

    bool PropertyStringBrowseEditHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyStringBrowseEditCtrl* GUI,
        [[maybe_unused]] const property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker blocker(GUI);
        GUI->SetValue(instance);
        return false;
    }

    PropertyFilePathStringCtrl::PropertyFilePathStringCtrl(QWidget* parent)
        : PropertyStringBrowseEditCtrl(parent)
    {
        m_browseEdit->setLineEditReadOnly(true);
        m_browseEdit->setAttachedButtonIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));
        SetEditButtonVisible(true);
        SetEditButtonEnabled(true);
    }

    void PropertyFilePathStringCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue)
    {
        PropertyStringBrowseEditCtrl::ConsumeAttribute(attrib, attrValue);

        if (attrib == AZ_CRC_CE("Title"))
        {
            if (AZStd::string value; attrValue->Read<decltype(value)>(value))
            {
                m_title = value;
            }
            return;
        }

        if (attrib == AZ_CRC_CE("Extensions") || attrib == AZ_CRC_CE("Extension"))
        {
            if (AZStd::string value; attrValue->Read<decltype(value)>(value))
            {
                AZStd::vector<AZStd::string> extensions;
                AZ::StringFunc::Tokenize(value, extensions, ";:, \t\r\n\\/|");

                m_extensions.clear();
                for (const auto& extension : extensions)
                {
                    m_extensions.emplace_back(AZStd::string{}, extension);
                }
                return;
            }

            if (AZStd::vector<AZStd::string> value; attrValue->Read<decltype(value)>(value))
            {
                m_extensions.clear();
                for (const auto& extension : value)
                {
                    m_extensions.emplace_back(AZStd::string{}, extension);
                }
                return;
            }

            if (AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> value; attrValue->Read<decltype(value)>(value))
            {
                m_extensions = value;
                return;
            }
            return;
        }
    }

    void PropertyFilePathStringCtrl::Edit()
    {
        if (const auto& paths = GetOpenFilePathsFromDialog({ GetValue() }, m_extensions, m_title, false); !paths.empty())
        {
            SetValue(GetPathWithAlias(paths.front()));
        }
    }

    QWidget* PropertyFilePathStringHandler::CreateGUI(QWidget* parent)
    {
        PropertyFilePathStringCtrl* newCtrl = aznew PropertyFilePathStringCtrl(parent);

        QObject::connect(
            newCtrl,
            &PropertyFilePathStringCtrl::ValueChanged,
            this,
            [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });

        return newCtrl;
    }

    void PropertyFilePathStringHandler::ConsumeAttribute(
        [[maybe_unused]] PropertyFilePathStringCtrl* GUI,
        [[maybe_unused]] AZ::u32 attrib,
        [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue,
        [[maybe_unused]] const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue);
    }

    void PropertyFilePathStringHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyFilePathStringCtrl* GUI,
        [[maybe_unused]] property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue();
    }

    bool PropertyFilePathStringHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyFilePathStringCtrl* GUI,
        [[maybe_unused]] const property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker blocker(GUI);
        GUI->SetValue(instance);
        return false;
    }

    PropertyMultiLineStringCtrl::PropertyMultiLineStringCtrl(QWidget* parent)
        : PropertyStringBrowseEditCtrl(parent)
    {
        m_browseEdit->setLineEditReadOnly(false);
        m_browseEdit->setAttachedButtonIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
        SetEditButtonVisible(true);
        SetEditButtonEnabled(true);
    }

    void PropertyMultiLineStringCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue)
    {
        PropertyStringBrowseEditCtrl::ConsumeAttribute(attrib, attrValue);
    }

    void PropertyMultiLineStringCtrl::Edit()
    {
        QDialog dialog(GetToolMainWindow());
        dialog.setWindowTitle(tr("Edit String Value"));
        dialog.setLayout(new QVBoxLayout());

        QTextEdit textEdit(&dialog);
        textEdit.setAcceptRichText(false);
        textEdit.setReadOnly(false);
        textEdit.setTabChangesFocus(false);
        textEdit.setTabStopDistance(4.0f);
        textEdit.setUndoRedoEnabled(true);
        textEdit.setLineWrapMode(QTextEdit::LineWrapMode::NoWrap);
        textEdit.setWordWrapMode(QTextOption::WrapMode::NoWrap);
        textEdit.setPlainText(GetValue().c_str());

        QDialogButtonBox buttonBox(&dialog);
        buttonBox.setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        dialog.layout()->addWidget(&textEdit);
        dialog.layout()->addWidget(&buttonBox);

        // Temporarily forcing a fixed size before showing it to compensate for window management centering and resizing the dialog.
        dialog.setFixedSize(800, 400);
        dialog.show();
        dialog.setMinimumSize(0, 0);
        dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

        if (dialog.exec() == QDialog::Accepted)
        {
            SetValue(textEdit.toPlainText().toUtf8().constData());
        }
    }

    QWidget* PropertyMultiLineStringHandler::CreateGUI(QWidget* parent)
    {
        PropertyMultiLineStringCtrl* newCtrl = aznew PropertyMultiLineStringCtrl(parent);

        QObject::connect(
            newCtrl,
            &PropertyMultiLineStringCtrl::ValueChanged,
            this,
            [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });

        return newCtrl;
    }

    void PropertyMultiLineStringHandler::ConsumeAttribute(
        [[maybe_unused]] PropertyMultiLineStringCtrl* GUI,
        [[maybe_unused]] AZ::u32 attrib,
        [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue,
        [[maybe_unused]] const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue);
    }

    void PropertyMultiLineStringHandler::WriteGUIValuesIntoProperty(
      [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyMultiLineStringCtrl* GUI,
        [[maybe_unused]] property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue();
    }

    bool PropertyMultiLineStringHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyMultiLineStringCtrl* GUI,
        [[maybe_unused]] const property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker blocker(GUI);
        GUI->SetValue(instance);
        return false;
    }

    PropertyMultiSelectSplitStringCtrl::PropertyMultiSelectSplitStringCtrl(QWidget* parent)
        : PropertyStringBrowseEditCtrl(parent)
    {
        m_browseEdit->setLineEditReadOnly(true);
        m_browseEdit->setAttachedButtonIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
        SetEditButtonVisible(true);
        SetEditButtonEnabled(true);
    }

    void PropertyMultiSelectSplitStringCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue)
    {
        PropertyStringBrowseEditCtrl::ConsumeAttribute(attrib, attrValue);

        if (attrib == AZ_CRC_CE("MultiSelectOptions"))
        {
            if (AZStd::string value; attrValue->Read<AZStd::string>(value))
            {
                SetOptions(value);
                return;
            }

            if (AZStd::vector<AZStd::string> value; attrValue->Read<AZStd::vector<AZStd::string>>(value))
            {
                SetOptionsVec(value);
                return;
            }
        }
    }

    void PropertyMultiSelectSplitStringCtrl::Edit()
    {
        AZStd::vector<AZStd::string> selections = GetValuesVec();
        if (GetStringListFromDialog(selections, GetOptionsVec(), "Select Options", true))
        {
            SetValuesVec(selections);
        }
    }

    void PropertyMultiSelectSplitStringCtrl::SetValues(const AZStd::string& values)
    {
        SetValue(values);
    }

    AZStd::string PropertyMultiSelectSplitStringCtrl::GetValues() const
    {
        return GetValue();
    }

    void PropertyMultiSelectSplitStringCtrl::SetValuesVec(const AZStd::vector<AZStd::string>& values)
    {
        AZStd::string value;
        AZ::StringFunc::Join(value, values, ", ");
        SetValues(value);
    }

    AZStd::vector<AZStd::string> PropertyMultiSelectSplitStringCtrl::GetValuesVec() const
    {
        AZStd::vector<AZStd::string> values;
        AZ::StringFunc::Tokenize(GetValues(), values, ";:, \t\r\n\\/|");
        return values;
    }

    void PropertyMultiSelectSplitStringCtrl::SetOptions(const AZStd::string& options)
    {
        m_options = options;
    }

    AZStd::string PropertyMultiSelectSplitStringCtrl::GetOptions() const
    {
        return m_options;
    }

    void PropertyMultiSelectSplitStringCtrl::SetOptionsVec(const AZStd::vector<AZStd::string>& options)
    {
        AZStd::string option;
        AZ::StringFunc::Join(option, options, ", ");
        SetOptions(option);
    }

    AZStd::vector<AZStd::string> PropertyMultiSelectSplitStringCtrl::GetOptionsVec() const
    {
        AZStd::vector<AZStd::string> options;
        AZ::StringFunc::Tokenize(GetOptions(), options, ";:, \t\r\n\\/|");
        return options;
    }

    QWidget* PropertyMultiSelectSplitStringHandler::CreateGUI(QWidget* parent)
    {
        PropertyMultiSelectSplitStringCtrl* newCtrl = aznew PropertyMultiSelectSplitStringCtrl(parent);

        QObject::connect(
            newCtrl,
            &PropertyMultiSelectSplitStringCtrl::ValueChanged,
            this,
            [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });

        return newCtrl;
    }

    void PropertyMultiSelectSplitStringHandler::ConsumeAttribute(
        [[maybe_unused]] PropertyMultiSelectSplitStringCtrl* GUI,
        [[maybe_unused]] AZ::u32 attrib,
        [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue,
        [[maybe_unused]] const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue);
    }

    void PropertyMultiSelectSplitStringHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyMultiSelectSplitStringCtrl* GUI,
        [[maybe_unused]] property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue();
    }

    bool PropertyMultiSelectSplitStringHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyMultiSelectSplitStringCtrl* GUI,
        [[maybe_unused]] const property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker blocker(GUI);
        GUI->SetValue(instance);
        return false;
    }

    QWidget* PropertyMultiSelectStringVectorHandler::CreateGUI(QWidget* parent)
    {
        PropertyMultiSelectSplitStringCtrl* newCtrl = aznew PropertyMultiSelectSplitStringCtrl(parent);

        QObject::connect(
            newCtrl,
            &PropertyMultiSelectSplitStringCtrl::ValueChanged,
            this,
            [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });

        return newCtrl;
    }

    void PropertyMultiSelectStringVectorHandler::ConsumeAttribute(
        [[maybe_unused]] PropertyMultiSelectSplitStringCtrl* GUI,
        [[maybe_unused]] AZ::u32 attrib,
        [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue,
        [[maybe_unused]] const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue);
    }

    void PropertyMultiSelectStringVectorHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyMultiSelectSplitStringCtrl* GUI,
        [[maybe_unused]] property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValuesVec();
    }

    bool PropertyMultiSelectStringVectorHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        [[maybe_unused]] PropertyMultiSelectSplitStringCtrl* GUI,
        [[maybe_unused]] const property_t& instance,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker blocker(GUI);
        GUI->SetValuesVec(instance);
        return false;
    }

    void RegisterStringBrowseEditHandler()
    {
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            aznew PropertyStringBrowseEditHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            aznew PropertyFilePathStringHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            aznew PropertyMultiLineStringHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            aznew PropertyMultiSelectSplitStringHandler());
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            aznew PropertyMultiSelectStringVectorHandler());
    }
} // namespace AtomToolsFramework

#include "Inspector/PropertyWidgets/moc_PropertyStringBrowseEditCtrl.cpp"
