/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystem/ParticleGraphicsViewRequestsBus.h>
#include <Document/ParticleDocumentBus.h>
#include <QAction>
#include <QByteArray>
#include <QCursor>
#include <QDesktopServices>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <Window/LevelOfDetailInspectorNotifyBus.h>
#include <Window/ParticleItemWidget.h>

namespace OpenParticleSystemEditor
{
    ParticleItemWidget::ParticleItemWidget(
        EffectorInspector* pDetail, OpenParticle::ParticleSourceData::DetailInfo* detail, OpenParticle::ParticleSourceData* sourceData, AZStd::string graphParentTitle, QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::particleItemWidget)
        , m_itemDetail(pDetail)
        , m_detail(detail)
        , m_pSourceData(sourceData)
        , m_graphParentTitle(graphParentTitle)
        , m_widgetSelect(false)
    {
        m_ui->setupUi(this);
        m_classChecks =
        {
            { PARTICLE_LINE_NAMES[WIDGET_LINE_SPAWN], m_ui->checkSpawn },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_PARTICLES], m_ui->checkParticles },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_LOCATION], m_ui->checkLocation },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_VELOCITY], m_ui->checkVelocity },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_COLOR], m_ui->checkColor },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_SIZE], m_ui->checkSize },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_FORCE], m_ui->checkForce },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_LIGHT], m_ui->checkLight },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_SUBUV], m_ui->checkSubUV },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_EVENT], m_ui->checkEvent },
            { PARTICLE_LINE_NAMES[WIDGET_LINE_RENDERER], m_ui->checkRenderer }
        };
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        this->setMouseTracking(true);
        this->setWindowFlags(Qt::FramelessWindowHint);
        this->setStyleSheet("background-color: #333333; border-radius: 8px;");
        SetLineWidgetIndex();
        SetLineWidgetParent();
        m_pSourceData->SelectClass(m_detail->m_name, PARTICLE_LINE_NAMES[WIDGET_LINE_EMITTER]);
        OnRelease();

        QObject::connect(m_ui->btnParticleName, SIGNAL(clicked()), this, SLOT(OnClickParticleName()));
        QObject::connect(m_ui->checkSolo, SIGNAL(clicked(bool)), this, SLOT(OnClickParticleSolo(bool)));
        QObject::connect(m_ui->checkParticleName, SIGNAL(clicked(bool)), this, SLOT(OnClickParticleName(bool)));
        QObject::connect(m_ui->checkSpawn, SIGNAL(clicked(bool)), this, SLOT(OnClickSpawn(bool)));
        QObject::connect(m_ui->checkParticles, SIGNAL(clicked(bool)), this, SLOT(OnClickParticles(bool)));
        QObject::connect(m_ui->checkLocation, SIGNAL(clicked(bool)), this, SLOT(OnClickLocation(bool)));
        QObject::connect(m_ui->checkVelocity, SIGNAL(clicked(bool)), this, SLOT(OnClickVelocity(bool)));
        QObject::connect(m_ui->checkColor, SIGNAL(clicked(bool)), this, SLOT(OnClickColor(bool)));
        QObject::connect(m_ui->checkSize, SIGNAL(clicked(bool)), this, SLOT(OnClickSize(bool)));
        QObject::connect(m_ui->checkForce, SIGNAL(clicked(bool)), this, SLOT(OnClickForce(bool)));
        QObject::connect(m_ui->checkLight, SIGNAL(clicked(bool)), this, SLOT(OnClickLight(bool)));
        QObject::connect(m_ui->checkSubUV, SIGNAL(clicked(bool)), this, SLOT(OnClickSubUV(bool)));
        QObject::connect(m_ui->checkEvent, SIGNAL(clicked(bool)), this, SLOT(OnClickEvent(bool)));
        QObject::connect(m_ui->checkRenderer, SIGNAL(clicked(bool)), this, SLOT(OnClickRenderer(bool)));
    }

    ParticleItemWidget::~ParticleItemWidget()
    {
    }

    void ParticleItemWidget::OnSelected()
    {
        m_ui->lineLeft->show();
        m_ui->lineRight->show();
        m_ui->lineTop->show();
        m_ui->lineBotton->show();
        m_widgetSelect = true;
    }

    void ParticleItemWidget::OnRelease()
    {
        m_ui->lineLeft->hide();
        m_ui->lineRight->hide();
        m_ui->lineTop->hide();
        m_ui->lineBotton->hide();
        m_widgetSelect = false;
    }

    void ParticleItemWidget::OnClickParticleSolo(bool checked)
    {
        AZStd::vector<AZStd::string> itemNames;
        EBUS_EVENT_ID_RESULT(itemNames, m_graphParentTitle, ParticleGraphicsViewRequestsBus, GetItemNamesByOrder);
        AZStd::string currDetailName = m_detail->m_name;
        if (checked)
        {
            for (auto& detailName : itemNames)
            {
                if (currDetailName.compare(detailName) == 0)
                {
                    m_pSourceData->UpdateDetailSoloState(detailName, true);
                    m_pSourceData->SelectDetail(detailName);
                    continue;
                }
                m_pSourceData->UpdateDetailSoloState(detailName, false);
                m_pSourceData->UnselectDetail(detailName);
            }
            EBUS_EVENT_ID(m_graphParentTitle, ParticleGraphicsViewRequestsBus, SetCheckedSolo, currDetailName, true, false);
            EBUS_EVENT_ID(m_graphParentTitle, ParticleGraphicsViewRequestsBus, SetChecked, currDetailName, true, false);
            ParticleItemWidget::m_ui->checkParticleName->setChecked(true);
        }
        else
        {
            for (auto& detailName : itemNames)
            {
                m_pSourceData->UpdateDetailSoloState(detailName, false);
                m_pSourceData->SelectDetail(detailName);
            }

            AZStd::string nullName;
            EBUS_EVENT_ID(m_graphParentTitle, ParticleGraphicsViewRequestsBus, SetCheckedSolo, nullName, false, false);
            EBUS_EVENT_ID(m_graphParentTitle, ParticleGraphicsViewRequestsBus, SetChecked, nullName, false, true);
        }
        EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, m_graphParentTitle);
        EBUS_EVENT_ID(m_graphParentTitle, ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void ParticleItemWidget::OnClickParticleName(bool bCheck)
    {
        if (bCheck)
        {
            AZStd::string name;
            if (m_pSourceData->SoloChecked(name))
            {
                m_pSourceData->UpdateDetailSoloState(name, false);
                AZStd::string nullName;
                EBUS_EVENT_ID(m_graphParentTitle, ParticleGraphicsViewRequestsBus, SetCheckedSolo, nullName, false, false);
            }
            AZStd::vector<AZStd::string> itemNames;
            EBUS_EVENT_ID_RESULT(itemNames, m_graphParentTitle, ParticleGraphicsViewRequestsBus, GetItemNamesByOrder);
            m_pSourceData->SelectDetail(m_detail->m_name);
            m_pSourceData->SortEmitters(itemNames);
        }
        else
        {
            m_pSourceData->UnselectDetail(m_detail->m_name);
        }
        EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, m_graphParentTitle);
        EBUS_EVENT_ID(m_graphParentTitle, ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
        ClickLineWidget(WIDGET_LINE_EMITTER);
        for (auto& it : m_classChecks)
        {
            it.second->setEnabled(bCheck);
        }
    }

    void ParticleItemWidget::AddRemoveEmitter(AZStd::string className, bool check, LineWidgetIndex index)
    {
        if (check)
        {
            m_pSourceData->SelectClass(m_detail->m_name, className);
        }
        else
        {
            m_pSourceData->UnselectClass(m_detail->m_name, className);
        }
        EBUS_EVENT_ID(m_graphParentTitle, ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
        ClickLineWidget(index);
    }

    void ParticleItemWidget::OnClickSpawn(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_SPAWN], bCheck, WIDGET_LINE_SPAWN);
    }

    void ParticleItemWidget::OnClickParticles(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_PARTICLES], bCheck, WIDGET_LINE_PARTICLES);
    }

    void ParticleItemWidget::OnClickLocation(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_LOCATION], bCheck, WIDGET_LINE_LOCATION);
    }

    void ParticleItemWidget::OnClickVelocity(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_VELOCITY], bCheck, WIDGET_LINE_VELOCITY);
    }

    void ParticleItemWidget::OnClickColor(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_COLOR], bCheck, WIDGET_LINE_COLOR);
    }

    void ParticleItemWidget::OnClickSize(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_SIZE], bCheck, WIDGET_LINE_SIZE);
    }

    void ParticleItemWidget::OnClickForce(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_FORCE], bCheck, WIDGET_LINE_FORCE);
    }

    void ParticleItemWidget::OnClickLight(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_LIGHT], bCheck, WIDGET_LINE_LIGHT);
    }

    void ParticleItemWidget::OnClickSubUV(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_SUBUV], bCheck, WIDGET_LINE_SUBUV);
    }

    void ParticleItemWidget::OnClickEvent(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_EVENT], bCheck, WIDGET_LINE_EVENT);
    }

    void ParticleItemWidget::OnClickRenderer(bool bCheck)
    {
        AddRemoveEmitter(PARTICLE_LINE_NAMES[WIDGET_LINE_RENDERER], bCheck, WIDGET_LINE_RENDERER);
    }

    void ParticleItemWidget::UpdataClassCheck(AZStd::string className, bool bCheck)
    {
        if (className != PARTICLE_LINE_NAMES[WIDGET_LINE_EMITTER])
        {
            m_classChecks.at(className)->setChecked(bCheck);
        }
    }

    void ParticleItemWidget::SetLineWidgetIndex()
    {
        m_ui->emitterWidget->SetLineIndex(WIDGET_LINE_EMITTER);
        m_ui->spawnWidget->SetLineIndex(WIDGET_LINE_SPAWN);
        m_ui->particlesWidget->SetLineIndex(WIDGET_LINE_PARTICLES);
        m_ui->locationWidget->SetLineIndex(WIDGET_LINE_LOCATION);
        m_ui->velocityWidget->SetLineIndex(WIDGET_LINE_VELOCITY);
        m_ui->colorWidget->SetLineIndex(WIDGET_LINE_COLOR);
        m_ui->sizeWidget->SetLineIndex(WIDGET_LINE_SIZE);
        m_ui->forceWidget->SetLineIndex(WIDGET_LINE_FORCE);
        m_ui->lightWidget->SetLineIndex(WIDGET_LINE_LIGHT);
        m_ui->subUVWidget->SetLineIndex(WIDGET_LINE_SUBUV);
        m_ui->eventWidget->SetLineIndex(WIDGET_LINE_EVENT);
        m_ui->rendererWidget->SetLineIndex(WIDGET_LINE_RENDERER);
        m_ui->titleWidget->SetLineIndex(WIDGET_LINE_TITLE);
    }

    void ParticleItemWidget::SetLineWidgetParent()
    {
        m_ui->emitterWidget->SetLineWidgetParent(this);
        m_ui->spawnWidget->SetLineWidgetParent(this);
        m_ui->particlesWidget->SetLineWidgetParent(this);
        m_ui->locationWidget->SetLineWidgetParent(this);
        m_ui->velocityWidget->SetLineWidgetParent(this);
        m_ui->colorWidget->SetLineWidgetParent(this);
        m_ui->sizeWidget->SetLineWidgetParent(this);
        m_ui->forceWidget->SetLineWidgetParent(this);
        m_ui->lightWidget->SetLineWidgetParent(this);
        m_ui->subUVWidget->SetLineWidgetParent(this);
        m_ui->eventWidget->SetLineWidgetParent(this);
        m_ui->rendererWidget->SetLineWidgetParent(this);
        m_ui->titleWidget->SetLineWidgetParent(this);
    }

    void ParticleItemWidget::ClickLineWidget([[maybe_unused]] LineWidgetIndex index)
    {
        m_pSourceData->m_currentDetailInfo = m_detail;
        m_itemDetail->m_detail = m_detail;
        m_itemDetail->m_sourceData = m_pSourceData;
        m_itemDetail->m_pItemWidget = (void*)this;
        m_itemDetail->OnClickLineWidget(index, m_graphParentTitle);
        OnSelected();
        m_itemDetail->SetCheckBoxEnabled(m_ui->checkParticleName->isChecked());
    }

    void ParticleItemWidget::mousePressEvent(QMouseEvent* event)
    {
        Q_UNUSED(event);
        ClickLineWidget(WIDGET_LINE_BLANK);
    }

    void ParticleItemWidget::UpdateParticleName()
    {
        m_ui->btnParticleName->setText(QString::fromUtf8(m_detail->m_name.c_str()));
    }

    QToolButton* ParticleItemWidget::GetBtnParticleName()
    {
        return m_ui->btnParticleName;
    }

    void ParticleItemWidget::painEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);
        QStyleOption opt;
        opt.initFrom(this);
        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    }

    void ParticleItemWidget::SetSoloChecked(bool checked) const
    {
        m_ui->checkSolo->setChecked(checked);
    }

    void ParticleItemWidget::SetDetailChecked(bool checked) const
    {
        m_ui->checkParticleName->setChecked(checked);
    }
} // namespace OpenParticleSystemEditor

#include <Window/moc_ParticleItemWidget.cpp>
