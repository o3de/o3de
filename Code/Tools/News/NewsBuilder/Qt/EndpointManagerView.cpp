/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EndpointManagerView.h"
#include "EndpointManager.h"
#include "EndpointEntryView.h"
#include "Qt/ui_EndpointManagerView.h"
#include "Qt/QCustomMessageBox.h"
#include "ResourceManagement/BuilderResourceManifest.h"

#include <QMessageBox>

namespace News
{

    EndpointManagerView::EndpointManagerView(QWidget* parent,
        BuilderResourceManifest& manifest)
        : QDialog(parent)
        , m_ui(new Ui::EndpointManagerViewWidget)
        , m_pManager(manifest.GetEndpointManager())
        , m_manifest(manifest)
    {
        m_ui->setupUi(this);

        for (auto endpoint : *m_pManager)
        {
            auto endpointView = AddEndpointEntry(endpoint);
            if (m_pManager->GetSelectedEndpoint() == endpoint)
            {
                selectEndpointSlot(endpointView);
            }
        }

        connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
        connect(m_ui->buttonAdd, &QPushButton::clicked, this, &EndpointManagerView::addEndpointSlot);
    }

    EndpointManagerView::~EndpointManagerView() {}

    EndpointEntryView* EndpointManagerView::AddEndpointEntry(Endpoint* pEndpoint)
    {
        auto endpointView = new EndpointEntryView(m_ui->endpointListContents, pEndpoint);
        auto layout = static_cast<QVBoxLayout*>(m_ui->endpointListContents->layout());
        layout->insertWidget(layout->count() - 2, endpointView);
        m_endpoints.append(endpointView);
        connect(endpointView, SIGNAL(selectSignal(EndpointEntryView*)),
            this, SLOT(selectEndpointSlot(EndpointEntryView*)));
        connect(endpointView, SIGNAL(deleteSignal(EndpointEntryView*)),
            this, SLOT(deleteEndpointSlot(EndpointEntryView*)));
        // this lets us modify newly-added widget in the same function
        qApp->processEvents();
        return endpointView;
    }

    void EndpointManagerView::Update() const
    {
        if (m_pSelectedEndpoint)
        {
            auto pEndpoint = m_pSelectedEndpoint->GetEndpoint();
            pEndpoint->SetName(m_ui->nameText->text());
            pEndpoint->SetAwsProfile(m_ui->awsProfileText->text());
            pEndpoint->SetUrl(m_ui->urlText->text());
            pEndpoint->SetBucket(m_ui->bucketText->text());
            m_pManager->SelectEndpoint(m_pSelectedEndpoint->GetEndpoint());
        }
        m_pManager->Save();
    }

    void EndpointManagerView::accept()
    {
        enum SyncResponse
        {
            Merge, ReplaceLocal, ReplaceRemote, Cancel
        };
        enum YesNoResponse
        {
            Yes, No
        };

        QCustomMessageBox msgBox(
            QCustomMessageBox::Question,
            tr("Pull data from Endpoint"),
            tr("You are changing an endpoint. "
                "What would you like to do with the data on the current endpoint?\n\n"
                "Merge - merge resources from the endpoint\n"
                "Replace Local - overwrite local resources with endpoint resources\n"
                "Replace Endpoint - overwrite endpoint resources with local resources\n"
                "Cancel - undo endpoint selection"
            ),
            this);
        msgBox.AddButton(tr("Merge"), Merge);
        msgBox.AddButton(tr("Replace Local"), ReplaceLocal);
        msgBox.AddButton(tr("Replace Endpoint"), ReplaceRemote);
        msgBox.AddButton(tr("Cancel"), Cancel);

        switch (msgBox.exec())
        {
        case Merge:
        {
            m_manifest.SetSyncType(SyncType::Merge);
            m_manifest.PersistLocalResources();
        }
        break;
        case ReplaceLocal:
        {
            if (m_manifest.HasChanges())
            {
                QCustomMessageBox msgBoxWarning(
                    QCustomMessageBox::Critical,
                    tr("Unsaved changes"),
                    tr("Local resources were modified but not published."
                        "Changing endpoints will cause unpublished work to be lost.\n\n"
                        "Would you like to proceed?"),
                    this);
                msgBoxWarning.AddButton(tr("Yes"), Yes);
                msgBoxWarning.AddButton(tr("No"), No);
                if (msgBoxWarning.exec() == No)
                {
                    return;
                }
            }
            m_manifest.SetSyncType(SyncType::Merge);
            m_manifest.Reset();
        }
        break;
        case ReplaceRemote:
        {
            QCustomMessageBox msgBoxWarning2(
                QCustomMessageBox::Critical,
                tr("Warning"),
                tr("This operation will IRREVERSABLY replace ALL resources on ") +
                m_pSelectedEndpoint->GetEndpoint()->GetName() +
                tr(" endpoint with local data.\n\n"
                    "Are you sure you'd like to proceed?"),
                this);
            msgBoxWarning2.AddButton(tr("Yes"), Yes);
            msgBoxWarning2.AddButton(tr("No"), No);
            if (msgBoxWarning2.exec() == No)
            {
                return;
            }
            m_manifest.SetSyncType(SyncType::Overwrite);
            m_manifest.PersistLocalResources();
        }
        break;
        case Cancel:
            return;
        }

        Update();

        QDialog::accept();
    }

    void EndpointManagerView::addEndpointSlot()
    {
        auto pEndpoint = new Endpoint(
            QObject::tr("New endpoint"),
            QObject::tr("Enter AWS profile name"),
            QObject::tr("Enter root URL"),
            QObject::tr("Enter s3 bucket name"));
        m_pManager->AddEndpoint(pEndpoint);
        auto endpointView = AddEndpointEntry(pEndpoint);
        selectEndpointSlot(endpointView);
    }

    void EndpointManagerView::selectEndpointSlot(EndpointEntryView* pEndpointView)
    {
        m_pSelectedEndpoint = pEndpointView;

        for (auto view : m_endpoints)
        {
            view->SetSelected(view == pEndpointView);
        }

        if (pEndpointView)
        {
            auto pEndpoint = pEndpointView->GetEndpoint();
            m_ui->nameText->setText(pEndpoint->GetName());
            m_ui->awsProfileText->setText(pEndpoint->GetAwsProfile());
            m_ui->urlText->setText(pEndpoint->GetUrl());
            m_ui->bucketText->setText(pEndpoint->GetBucket());
        }
        else
        {
            m_ui->nameText->setText("");
            m_ui->awsProfileText->setText("");
            m_ui->urlText->setText("");
            m_ui->bucketText->setText("");
        }
    }

    void EndpointManagerView::deleteEndpointSlot(EndpointEntryView* pEndpointView)
    {
        m_pManager->RemoveEndpoint(pEndpointView->GetEndpoint());
        m_endpoints.removeAll(pEndpointView);

        if (pEndpointView == m_pSelectedEndpoint)
        {
            if (m_endpoints.count() > 0)
            {
                selectEndpointSlot(m_endpoints[0]);
            }
            else
            {
                selectEndpointSlot(nullptr);
            }
        }

        delete pEndpointView;
    }


#include "Qt/moc_EndpointManagerView.cpp"

}
