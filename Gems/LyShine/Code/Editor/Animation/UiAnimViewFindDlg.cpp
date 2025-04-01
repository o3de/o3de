/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "UiAnimViewFindDlg.h"
#include "UiAnimViewDialog.h"
#include "UiAnimViewSequenceManager.h"

#include <LyShine/Animation/IUiAnimation.h>
#include <QtUtilWin.h>

#include <QListWidgetItem>

#include <Editor/Animation/ui_UiAnimViewFindDlg.h>

/////////////////////////////////////////////////////////////////////////////
// CUiAnimViewFindDlg dialog


CUiAnimViewFindDlg::CUiAnimViewFindDlg(const char* title, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::UiAnimViewFindDlg)
{
    setWindowTitle(title);

    m_tvDlg = 0;
    m_numSeqs = 0;

    ui->setupUi(this);
    ui->LIST->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->OK, &QPushButton::clicked, this, &CUiAnimViewFindDlg::OnOK);
    connect(ui->CANCEL, &QPushButton::clicked, this, &CUiAnimViewFindDlg::OnCancel);
    connect(ui->FILTER, &QLineEdit::textEdited, this, &CUiAnimViewFindDlg::OnFilterChange);
    connect(ui->LIST, &QListWidget::itemDoubleClicked, this, &CUiAnimViewFindDlg::OnItemDoubleClicked);

    FillData();
}

CUiAnimViewFindDlg::~CUiAnimViewFindDlg()
{
}

void CUiAnimViewFindDlg::FillData()
{
    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    m_numSeqs = 0;
    m_objs.resize(0);
    for (int k = 0; k < animationSystem->GetNumSequences(); ++k)
    {
        IUiAnimSequence* seq = animationSystem->GetSequence(k);
        for (int i = 0; i < seq->GetNodeCount(); i++)
        {
            IUiAnimNode* pNode = seq->GetNode(i);
            ObjName obj;
            obj.m_objName = QString::fromUtf8(pNode->GetName().c_str());
            obj.m_directorName = pNode->HasDirectorAsParent() ? QString::fromUtf8(pNode->HasDirectorAsParent()->GetName().c_str()) : "";
            AZStd::string fullname = seq->GetName();
            obj.m_seqName = fullname.c_str();
            m_objs.push_back(obj);
        }
        m_numSeqs++;
    }
    FillList();
}


void CUiAnimViewFindDlg::Init(CUiAnimViewDialog* tvDlg)
{
    m_tvDlg = tvDlg;
}

void CUiAnimViewFindDlg::FillList()
{
    QString filter = ui->FILTER->text();
    ui->LIST->clear();

    for (int i = 0; i < m_objs.size(); i++)
    {
        ObjName pObj = m_objs[i];
        if (filter.isEmpty() || pObj.m_objName.contains(filter, Qt::CaseInsensitive))
        {
            QString text = pObj.m_objName;
            if (!pObj.m_directorName.isEmpty())
            {
                text += " (";
                text += pObj.m_directorName;
                text += ")";
            }
            if (m_numSeqs > 1)
            {
                text += " / ";
                text += pObj.m_seqName;
            }
            ui->LIST->addItem(text);
        }
    }
    ui->LIST->setCurrentRow(0);
}

void CUiAnimViewFindDlg::OnOK()
{
    ProcessSel();
    accept();
}

void CUiAnimViewFindDlg::OnCancel()
{
    reject();
}

void CUiAnimViewFindDlg::OnFilterChange([[maybe_unused]] const QString& text)
{
    FillList();
}

void CUiAnimViewFindDlg::ProcessSel()
{
    QList<QListWidgetItem*> selection = ui->LIST->selectedItems();

    if (selection.size() != 1)
    {
        return;
    }
    int index = ui->LIST->row(selection.first());

    if (index >= 0 && m_tvDlg)
    {
        ObjName object = m_objs[index];

        const CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
        CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(object.m_seqName);

        if (pSequence)
        {
            CUiAnimationContext* pAnimationContext = nullptr;
            UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
            pAnimationContext->SetSequence(pSequence, false, false, true);

            CUiAnimViewAnimNode* pParentDirector = pSequence;
            CUiAnimViewAnimNodeBundle foundDirectorNodes = pSequence->GetAnimNodesByName(object.m_directorName.toUtf8().data());
            if (foundDirectorNodes.GetCount() > 0 && foundDirectorNodes.GetNode(0)->GetType() == eUiAnimNodeType_Director)
            {
                pParentDirector = foundDirectorNodes.GetNode(0);
            }

            CUiAnimViewAnimNodeBundle foundNodes = pParentDirector->GetAnimNodesByName(object.m_objName.toUtf8().data());

            const uint numNodes = foundNodes.GetCount();
            for (uint i = 0; i < numNodes; ++i)
            {
                foundNodes.GetNode(i)->SetSelected(true);
            }
        }
    }
}

void CUiAnimViewFindDlg::OnItemDoubleClicked()
{
    ProcessSel();
}

#include <Animation/moc_UiAnimViewFindDlg.cpp>
