/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

// CTrackViewFindDlg dialog


CTrackViewFindDlg::CTrackViewFindDlg(const char* title, QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , ui(new Ui::TrackViewFindDlg)
{
    setWindowTitle(title);

    m_tvDlg = nullptr;
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

    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        for (int k = 0; k < movieSystem->GetNumSequences(); ++k)
        {
            IAnimSequence* seq = movieSystem->GetSequence(k);
            for (int i = 0; i < seq->GetNodeCount(); i++)
            {
                IAnimNode* pNode = seq->GetNode(i);
                ObjName obj;
                obj.m_objName = pNode->GetName();
                obj.m_directorName = pNode->HasDirectorAsParent() ? pNode->HasDirectorAsParent()->GetName() : "";
                AZStd::string fullname = seq->GetName();
                obj.m_seqName = fullname.c_str();
                m_objs.push_back(obj);
            }
            m_numSeqs++;
        }
        FillList();
    }
}

void CTrackViewFindDlg::Init(CTrackViewDialog* tvDlg)
{
    m_tvDlg = tvDlg;
}

void CTrackViewFindDlg::FillList()
{
    QString filter = ui->FILTER->text();
    ui->LIST->clear();
    m_objsSourceIndex.clear();

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
            m_objsSourceIndex.push_back(i);
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
        int sourceIndex = m_objsSourceIndex[index];
        ObjName object = m_objs[sourceIndex];

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
            uint selectedNodeIndex = 0;
            if (numNodes > 1)
            {
                for (int i = 0; i < sourceIndex; ++i)
                {
                    if (pParentDirector != pSequence && object.m_directorName != m_objs[i].m_directorName)
                    {
                        continue;
                    }
                    if (m_objs[i].m_objName == object.m_objName)
                    {
                        selectedNodeIndex++;
                    }
                }
            }

            if (selectedNodeIndex < numNodes)
            {
                CTrackViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();
                for (uint i = 0, numAnimNodes = animNodes.GetCount(); i < numAnimNodes; ++i)
                {
                    animNodes.GetNode(i)->SetSelected(false);
                }

                foundNodes.GetNode(selectedNodeIndex)->SetSelected(true);
            }
        }
    }
}

void CTrackViewFindDlg::OnItemDoubleClicked()
{
    ProcessSel();
}

#include <TrackView/moc_TrackViewFindDlg.cpp>
