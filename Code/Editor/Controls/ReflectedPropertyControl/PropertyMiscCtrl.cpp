/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "PropertyMiscCtrl.h"

// Qt
#include <QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QLineEdit>
#include <QtWidgets/QToolButton>
#include <QtCore/QTimer>
#include <QtUtilWin.h>

// Editor
#include "GenericSelectItemDialog.h"
#include "QtViewPaneManager.h"


UserPropertyEditor::UserPropertyEditor(QWidget *pParent /*= nullptr*/)
    : QWidget(pParent)
    , m_canEdit(false)
    , m_useTree(false)
{
    m_valueLabel = new QLabel;

    QToolButton *mainButton = new QToolButton;
    mainButton->setText("..");
    connect(mainButton, &QToolButton::clicked, this, &UserPropertyEditor::onEditClicked);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_valueLabel, 1);
    mainLayout->addWidget(mainButton);
    mainLayout->setContentsMargins(1, 1, 1, 1);
}

void UserPropertyEditor::SetValue(const QString &value, bool notify /*= true*/)
{
    if (m_value != value)
    {
        m_value = value;
        m_valueLabel->setText(m_value);
        if (notify)
        {
            emit ValueChanged(m_value);
        }
    }

}

void UserPropertyEditor::SetData(bool canEdit, bool useTree, const QString &treeSeparator, const QString &dialogTitle, const std::vector<IVariable::IGetCustomItems::SItem>& items)
{
    m_canEdit = canEdit;
    m_useTree = useTree;
    m_treeSeparator = treeSeparator;
    m_dialogTitle = dialogTitle;
    m_items = items;
}

void UserPropertyEditor::onEditClicked()
{
    // call the user supplied callback to fill-in items and get dialog title
    emit RefreshItems();
    if (m_canEdit) // if func didn't veto, show the dialog
    {
        CGenericSelectItemDialog gtDlg;
        if (m_useTree)
        {
            gtDlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
            if (!m_treeSeparator.isEmpty())
            {
                gtDlg.SetTreeSeparator(m_treeSeparator);
            }
        }
        gtDlg.SetItems(m_items);
        if (m_dialogTitle.isEmpty() == false)
            gtDlg.setWindowTitle(m_dialogTitle);
        gtDlg.PreSelectItem(GetValue());
        if (gtDlg.exec() == QDialog::Accepted)
        {
            QString selectedItemStr = gtDlg.GetSelectedItem();

            if (selectedItemStr.isEmpty() == false)
            {
                SetValue(selectedItemStr);
            }
        }
    }
}


QWidget* UserPopupWidgetHandler::CreateGUI(QWidget *pParent)
{
    UserPropertyEditor* newCtrl = aznew UserPropertyEditor(pParent);
    connect(newCtrl, &UserPropertyEditor::ValueChanged, newCtrl, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
        });

    return newCtrl;
}

void UserPopupWidgetHandler::ConsumeAttribute(UserPropertyEditor* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    Q_UNUSED(GUI);
    Q_UNUSED(attrib);
    Q_UNUSED(attrValue);
    Q_UNUSED(debugName);
}

void UserPopupWidgetHandler::WriteGUIValuesIntoProperty(size_t index, UserPropertyEditor* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarUser val = instance;
    val.m_value = GUI->GetValue().toUtf8().data();
    instance = static_cast<property_t>(val);
}

bool UserPopupWidgetHandler::ReadValuesIntoGUI(size_t index, UserPropertyEditor* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarUser val = instance;

    assert(val.m_itemNames.size() == val.m_itemDescriptions.size());

    std::vector<IVariable::IGetCustomItems::SItem> items(val.m_itemNames.size());
    int i = -1;
    std::generate(items.begin(), items.end(), [&val, &i]() { ++i; return IVariable::IGetCustomItems::SItem(val.m_itemNames[i].c_str(), val.m_itemDescriptions[i].c_str());});

    GUI->SetData(val.m_enableEdit, val.m_useTree, val.m_treeSeparator.c_str(), val.m_dialogTitle.c_str(), items);
    GUI->SetValue(val.m_value.c_str(), false);
    return false;
}

#include <Controls/ReflectedPropertyControl/moc_PropertyMiscCtrl.cpp>

QWidget* FloatCurveHandler::CreateGUI(QWidget *pParent)
{
    CSplineCtrl *cSpline = new CSplineCtrl(pParent);
    cSpline->SetUpdateCallback([this](CSplineCtrl* spl) { OnSplineChange(spl); });
    cSpline->SetTimeRange(0, 1);
    cSpline->SetValueRange(0, 1);
    cSpline->SetGrid(12, 12);
    cSpline->setFixedHeight(52);
    return cSpline;
}
void FloatCurveHandler::OnSplineChange(CSplineCtrl*)
{
    //AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
    //    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, splineWidget);
}

void FloatCurveHandler::ConsumeAttribute(CSplineCtrl *, AZ::u32, AzToolsFramework::PropertyAttributeReader*, const char*)
{}

void FloatCurveHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, [[maybe_unused]] CSplineCtrl* GUI, [[maybe_unused]] property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    //nothing to do here. the spline itself will have it's new values.
}

bool FloatCurveHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, CSplineCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    GUI->SetSpline(reinterpret_cast<ISplineInterpolator*>(instance.m_spline));
    return false;
}
