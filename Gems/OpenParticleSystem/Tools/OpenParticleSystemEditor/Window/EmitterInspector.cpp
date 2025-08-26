/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EmitterInspector.h>
#include <QAction>
#include <QByteArray>
#include <QMainWindow>
#include <QCursor>
#include <QDesktopServices>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QSettings>
#include <AzCore/Casting/numeric_cast.h>

namespace OpenParticleSystemEditor
{
    EmitterInspector::EmitterInspector(EffectorInspector* pViewDetail, QWidget* parent, QString windowTitle)
        : QWidget(parent)
        , m_ui(new Ui::EmitterInspector)
    {
        m_ui->setupUi(this);
        this->setWindowTitle(windowTitle);
        m_ui->graphicsView->SetEBusId(windowTitle);
        scene = new ParticleGraphicsScence();
        m_ui->graphicsView->setScene(scene);
        m_ui->graphicsView->m_viewDetail = pViewDetail;
        ParticleDocumentNotifyBus::Handler::BusConnect();
    }

    EmitterInspector::~EmitterInspector()
    {
        ParticleDocumentNotifyBus::Handler::BusDisconnect();
    }

    void EmitterInspector::OnParticleSourceDataLoaded(OpenParticle::ParticleSourceData* particleSourceData, AZStd::string particleAssetPath) const
    {
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(particleAssetPath.c_str(), fileName);

        if (QString::compare(fileName.c_str(), this->windowTitle()) != 0)
        {
            return;
        }

        m_ui->graphicsView->scene()->clear();
        m_ui->graphicsView->m_itemCount = 0;
        m_ui->graphicsView->m_SourceData = particleSourceData;
        m_ui->graphicsView->m_viewDetail->Init(fileName);

        for (auto iter : particleSourceData->m_details)
        {
            m_ui->graphicsView->AddItemFromData(iter);
        }
        m_ui->graphicsView->ReadAllViewItemPos(particleAssetPath);
    }
} // namespace OpenParticleSystemEditor

#include <Window/moc_EmitterInspector.cpp>
