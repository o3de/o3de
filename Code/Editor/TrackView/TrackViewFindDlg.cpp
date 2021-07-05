/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewFindDlg.h"

// Editor
#include "TrackViewSequenceManager.h"
#include "AnimationContext.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <TrackView/ui_TrackViewFindDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "Maestro/Types/AnimNodeType.h"

/////////////////////////////////////////////////////////////////////////////
// CTrackViewFindDlg dialog


CTrackViewFindDlg::CTrackViewFindDlg(const char* title, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::TrackViewFindDlg)
{
    setWindowTitle(title);

    m_tvDlg = 0;
    m_numSeqs = 0;

    ui->setupUi(this);
    ui->LIST->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->OK, &QPushButton::clicked, this, &CTrackViewFindDlg::OnOK);
    connect(ui->CANCEL, &QPushButton::clicked, this, &CTrackViewFindDlg::OnCancel);
    connect(ui->FILTER, &QLineEdit::textEdited, this, &CTrackViewFindDlg::OnFilterChange);
    connect(ui->LIST, &QListWidget::itemDoubleClicked, this, &CTrackViewFindDlg::OnItemDoubleClicked);

    FillData();
}

CTrackViewFindDlg::~CTrackViewFindDlg()
{
}

void CTrackViewFindDlg::FillData()
{
    m_numSeqs = 0;
    m_objs.resize(0);
    for (int k = 0; k < GetIEditor()->GetMovieSystem()->GetNumSequences(); ++k)
    {
        IAnimSequence* seq = GetIEditor()->GetMovieSystem()->GetSequence(k);
        for (int i = 0; i < seq->GetNodeCount(); i++)
        {
            IAnimNode* pNode = seq->GetNode(i);
            ObjName obj;
            obj.m_objName = pNode->GetName();
            obj.m_directorName = pNode->HasDirectorAsParent() ? pNode->HasDirectorAsParent()->GetName() : "";
            string fullname = seq->GetName();
            obj.m_seqName = fullname.c_str();
            m_objs.push_back(obj);
        }
        m_numSeqs++;
    }
    FillList();
}


void CTrackViewFindDlg::Init(CTrackViewDialog* tvDlg)
{
    m_tvDlg = tvDlg;
}

void CTrackViewFindDlg::FillList()
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

void CTrackViewFindDlg::OnOK()
{
    ProcessSel();
    accept();
}

void CTrackViewFindDlg::OnCancel()
{
    reject();
}

void CTrackViewFindDlg::OnFilterChange([[maybe_unused]] const QString& text)
{
    FillList();
}

void CTrackViewFindDlg::ProcessSel()
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

        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = pSequenceManager->GetSequenceByName(object.m_seqName);

        if (pSequence)
        {
            CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
            pAnimationContext->SetSequence(pSequence, false, false);

            CTrackViewAnimNode* pParentDirector = pSequence;
            CTrackViewAnimNodeBundle foundDirectorNodes = pSequence->GetAnimNodesByName(object.m_directorName.toUtf8().data());
            if (foundDirectorNodes.GetCount() > 0 && foundDirectorNodes.GetNode(0)->GetType() == AnimNodeType::Director)
            {
                pParentDirector = foundDirectorNodes.GetNode(0);
            }

            CTrackViewAnimNodeBundle foundNodes = pParentDirector->GetAnimNodesByName(object.m_objName.toUtf8().data());

            const uint numNodes = foundNodes.GetCount();
            for (uint i = 0; i < numNodes; ++i)
            {
                foundNodes.GetNode(i)->SetSelected(true);
            }
        }
    }
}

void CTrackViewFindDlg::OnItemDoubleClicked()
{
    ProcessSel();
}

#include <TrackView/moc_TrackViewFindDlg.cpp>
