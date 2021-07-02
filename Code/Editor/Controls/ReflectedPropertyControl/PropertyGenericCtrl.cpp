/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "PropertyGenericCtrl.h"

// Qt
#include <QMessageBox>
#include <QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QLineEdit>
#include <QStringListModel>
#include <QToolButton>

// CryCommon
#include <CryCommon/ILocalizationManager.h>

// Editor
#include "SelectLightAnimationDialog.h"
#include "SelectSequenceDialog.h"
#include "SelectEAXPresetDlg.h"
#include "QtViewPaneManager.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <QtWidgets/QListView>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


GenericPopupPropertyEditor::GenericPopupPropertyEditor(QWidget *pParent, bool showTwoButtons)
    :QWidget(pParent)
{
    m_valueLabel = new QLabel;

    QToolButton *mainButton = new QToolButton;
    mainButton->setAutoRaise(true);
    mainButton->setIcon(QIcon(QStringLiteral(":/stylesheet/img/UI20/browse-edit.svg")));
    connect(mainButton, &QToolButton::clicked, this, &GenericPopupPropertyEditor::onEditClicked);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_valueLabel, 1);
    mainLayout->addWidget(mainButton);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    if (showTwoButtons)
    {
        QToolButton *button2 = new QToolButton;
        button2->setAutoRaise(true);
        button2->setIcon(QIcon(QStringLiteral(":/stylesheet/img/UI20/more.svg")));
        connect(button2, &QToolButton::clicked, this, &GenericPopupPropertyEditor::onButton2Clicked);
        mainLayout->insertWidget(1, button2);
    }
}

void GenericPopupPropertyEditor::SetValue(const QString &value, bool notify)
{
    if (m_value != value)
    {
        m_value = value;
        m_valueLabel->setText(m_value);
        if (notify)
            emit ValueChanged(m_value);
    }
}

void GenericPopupPropertyEditor::SetPropertyType(PropertyType type)
{
    m_propertyType = type;
}

void ReverbPresetPropertyEditor::onEditClicked()
{
    CSelectEAXPresetDlg PresetDlg(this);
    PresetDlg.SetCurrPreset(GetValue());
    if (PresetDlg.exec() == QDialog::Accepted)
    {
        SetValue(PresetDlg.GetCurrPreset());
    }
}

void SequencePropertyEditor::onEditClicked()
{
    CSelectSequenceDialog gtDlg(this);
    gtDlg.PreSelectItem(GetValue());
    if (gtDlg.exec() == QDialog::Accepted)
        SetValue(gtDlg.GetSelectedItem());
}

void SequenceIdPropertyEditor::onEditClicked()
{
    CSelectSequenceDialog gtDlg;
    uint32 id = GetValue().toUInt();
    IAnimSequence *pSeq = GetIEditor()->GetMovieSystem()->FindSequenceById(id);
    if (pSeq)
        gtDlg.PreSelectItem(pSeq->GetName());
    if (gtDlg.exec() == QDialog::Accepted)
    {
        pSeq = GetIEditor()->GetMovieSystem()->FindLegacySequenceByName(gtDlg.GetSelectedItem().toUtf8().data());
        assert(pSeq);
        if (pSeq->GetId() > 0)
        {
            // This sequence is a new one with a valid ID.
            SetValue(QString::number(pSeq->GetId()));
        }
        else
        {
            // This sequence is an old one without an ID.
            QMessageBox::warning(this, tr("Old Sequence"), tr("This is an old sequence without an ID.\nSo it cannot be used with the new ID-based linking."));
        }
    }
}

void LocalStringPropertyEditor::onEditClicked()
{
    std::vector<IVariable::IGetCustomItems::SItem> items;
    ILocalizationManager* pMgr = gEnv->pSystem->GetLocalizationManager();
    if (!pMgr)
        return;
    int nCount = pMgr->GetLocalizedStringCount();
    if (nCount <= 0)
        return;
    items.reserve(nCount);
    IVariable::IGetCustomItems::SItem item;
    SLocalizedInfoEditor sInfo;
    for (int i = 0; i < nCount; ++i)
    {
        if (pMgr->GetLocalizedInfoByIndex(i, sInfo))
        {
            item.desc = tr("English Text:\r\n");
            item.desc += QString::fromWCharArray(Unicode::Convert<wstring>(sInfo.sUtf8TranslatedText).c_str());
            item.name = sInfo.sKey;
            items.push_back(item);
        }
    }
    CGenericSelectItemDialog gtDlg;
    const bool bUseTree = true;
    if (bUseTree)
    {
        gtDlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
        gtDlg.SetTreeSeparator("/");
    }
    gtDlg.SetItems(items);
    gtDlg.setWindowTitle(tr("Choose Localized String"));
    QString preselect = GetValue();
    if (!preselect.isEmpty() && preselect.at(0) == '@')
        preselect = preselect.mid(1);
    gtDlg.PreSelectItem(preselect);
    if (gtDlg.exec() == QDialog::Accepted)
    {
        preselect = "@";
        preselect += gtDlg.GetSelectedItem();
        SetValue(preselect);
    }
}

void LightAnimationPropertyEditor::onEditClicked()
{
    // First, check if there is any light animation defined.
    bool bLightAnimationExists = false;
    IMovieSystem *pMovieSystem = GetIEditor()->GetMovieSystem();
    for (int i = 0; i < pMovieSystem->GetNumSequences(); ++i)
    {
        IAnimSequence *pSequence = pMovieSystem->GetSequence(i);
        if (pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet)
        {
            bLightAnimationExists = pSequence->GetNodeCount() > 0;
            break;
        }
    }

    if (bLightAnimationExists) // If exists, show the selection dialog.
    {
        CSelectLightAnimationDialog dlg;
        dlg.PreSelectItem(GetValue());
        if (dlg.exec() == QDialog::Accepted)
            SetValue(dlg.GetSelectedItem());
    }
    else                       // If not, remind the user of creating one in TrackView.
    {
        QMessageBox::warning(this, tr("No Available Animation"), tr("There is no available light animation.\nPlease create one in TrackView, first."));
    }
}

ListEditWidget::ListEditWidget(QWidget *pParent /*= nullptr*/)
    :QWidget(pParent)
{
    m_valueEdit = new QLineEdit;

    m_model = new QStringListModel(this);

    m_listView = new QListView;
    m_listView->setModel(m_model);
    m_listView->setMaximumHeight(50);
    m_listView->setVisible(false);

    QToolButton *expandButton = new QToolButton();
    expandButton->setCheckable(true);
    expandButton->setText("+");
    
    QToolButton *editButton = new QToolButton();
    editButton->setText("..");

    connect(editButton, &QAbstractButton::clicked, this, &ListEditWidget::OnEditClicked);
    connect(expandButton, &QAbstractButton::toggled, m_listView, &QWidget::setVisible);

    connect(m_model, &QAbstractItemModel::dataChanged, this, &ListEditWidget::OnModelDataChange);
    connect(m_valueEdit, &QLineEdit::editingFinished, this, [this](){SetValue(m_valueEdit->text(), true); } );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(expandButton);
    topLayout->addWidget(m_valueEdit,1);
    topLayout->addWidget(editButton);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_listView,1);
    mainLayout->setContentsMargins(1,1,1,1);
}

void ListEditWidget::SetValue(const QString &value, bool notify /*= true*/)
{
    if (m_value != value)
    {
        m_value = value;
        m_valueEdit->setText(value);
        QStringList list = m_value.split(",", Qt::SkipEmptyParts);
        m_model->setStringList(list);

        if (notify)
            emit ValueChanged(m_value);
    }
}


void ListEditWidget::OnModelDataChange()
{
    m_value = m_model->stringList().join(",");
    m_valueEdit->setText(m_value);
    emit ValueChanged(m_value);
}


QWidget* ListEditWidget::GetFirstInTabOrder()
{
    return m_valueEdit;
}

QWidget* ListEditWidget::GetLastInTabOrder()
{
    return m_listView;
}

#include <Controls/ReflectedPropertyControl/moc_PropertyGenericCtrl.cpp>
