/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ParticleGraphicsView.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <Document/ParticleDocumentBus.h>
#include <Window/LevelOfDetailInspectorNotifyBus.h>
#include <QMessageBox>
#include <QSettings>
#include <QFile>

namespace OpenParticleSystemEditor
{
    const float VIEW_DRAG_IGNORE = 0.005f;
    const float VIEW_WHEEL_SCALE = 0.00125f;
    ParticleGraphicsView::ParticleGraphicsView(QWidget* parent)
        : QGraphicsView(parent)
        , m_minZoom(0.1f)
        , m_maxZoom(2.0f)
        , m_anchorPointX(0.0f)
        , m_anchorPointY(0.0f)
        , m_scale(1.0)
        , m_viewDetail(nullptr)
        , m_itemCount(0)
        , m_SourceData(nullptr)
    {
        SetupUi();
    }

    ParticleGraphicsView::~ParticleGraphicsView()
    {
        ParticleGraphicsViewRequestsBus::Handler::BusDisconnect(m_busID);
    }

    void ParticleGraphicsView::SetupUi()
    {
        setContentsMargins(0, 0, 0, 0);
        setAlignment(Qt::AlignCenter);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setDragMode(QGraphicsView::RubberBandDrag);
        setMouseTracking(true);
        setStyleSheet("background-color: #2D2D2D;");
        m_pActAddItem = new QAction(tr("Add Emitter"));
        m_pActDelItem = new QAction(tr("Delete Emitter"));
        m_pActCopyItem = new QAction(tr("Copy Emitter"));
        m_pActPasteItem = new QAction(tr("Paste Emitter"));

        setContextMenuPolicy(Qt::ActionsContextMenu);
        connect(m_pActAddItem, SIGNAL(triggered()), this, SLOT(AddItem()));
        connect(m_pActDelItem, SIGNAL(triggered()), this, SLOT(DeleteItem()));
        connect(m_pActCopyItem, SIGNAL(triggered()), this, SLOT(CopyItem()));
        connect(m_pActPasteItem, SIGNAL(triggered()), this, SLOT(PasteItem()));
    }

    void ParticleGraphicsView::SetEBusId(QString windowTitle)
    {
        m_busID = windowTitle.toStdString().c_str();
        ParticleGraphicsViewRequestsBus::Handler::BusConnect(m_busID);
    }

    void ParticleGraphicsView::contextMenuEvent(QContextMenuEvent* contextMenuEvent)
    {
        if (QContextMenuEvent::Mouse == contextMenuEvent->reason())
        {
            return;
        }
        QGraphicsView::contextMenuEvent(contextMenuEvent);
    }

    void ParticleGraphicsView::mousePressEvent(QMouseEvent* mouseEvent)
    {
        if (mouseEvent->buttons() != mouseEvent->button())
        {
            if (!m_checkForDrag && (Qt::RightButton == mouseEvent->button()))
            {
            }
        }

        if ((Qt::MiddleButton == mouseEvent->button() || Qt::RightButton == mouseEvent->button()))
        {
            m_initialClick = mouseEvent->pos();
            m_checkForDrag = true;
            mouseEvent->accept();
            QMouseEvent customMouseEvent(mouseEvent->type(), mouseEvent->pos(), Qt::LeftButton, Qt::LeftButton, mouseEvent->modifiers());
            SetItemSelected();
            QGraphicsView::mousePressEvent(&customMouseEvent);
            QGraphicsView::mouseReleaseEvent(&customMouseEvent);
            return;
        }
        else if (Qt::LeftButton == mouseEvent->button())
        {
            m_checkForEdges = true;
            m_checkForMove = true;
        }
        SetItemSelected();
        QGraphicsView::mousePressEvent(mouseEvent);
        SetItemZValue();
    }

    void ParticleGraphicsView::mouseMoveEvent(QMouseEvent* mouseEvent)
    {
        if ((mouseEvent->buttons() & Qt::LeftButton) != Qt::NoButton)
        {
            if (m_checkForMove && this->scene()->mouseGrabberItem() != nullptr)
            {
                m_checkForMove = false;
                EBUS_EVENT_ID(m_busID, ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            }
        }

        if ((mouseEvent->buttons() & (Qt::RightButton | Qt::MiddleButton)) == Qt::NoButton)
        {
            m_checkForDrag = false;
        }
        else if (m_checkForDrag && isInteractive())
        {
            mouseEvent->accept();

            if ((m_initialClick - mouseEvent->pos()).manhattanLength() > (width() * VIEW_DRAG_IGNORE + height() * VIEW_DRAG_IGNORE))
            {
                setDragMode(QGraphicsView::ScrollHandDrag);
                setInteractive(false);

                QMouseEvent startPressMouseEvent(
                    QEvent::MouseButtonPress, m_initialClick, Qt::LeftButton, Qt::LeftButton, mouseEvent->modifiers());
                QGraphicsView::mousePressEvent(&startPressMouseEvent);

                QMouseEvent customMouseEvent(
                    mouseEvent->type(), mouseEvent->pos(), Qt::LeftButton, Qt::LeftButton, mouseEvent->modifiers());
                QGraphicsView::mousePressEvent(&customMouseEvent);
            }
            return;
        }

        QGraphicsView::mouseMoveEvent(mouseEvent);
    }

    void ParticleGraphicsView::mouseReleaseEvent(QMouseEvent* mouseEvent)
    {
        if (Qt::RightButton == mouseEvent->button())
        {
            if (m_SourceData != nullptr &&
                (m_initialClick - mouseEvent->pos()).manhattanLength() <= (width() * VIEW_DRAG_IGNORE + height() * VIEW_DRAG_IGNORE))
            {
                ShowMenu(mouseEvent->pos());
            }
        }
        if (Qt::RightButton == mouseEvent->button() || Qt::MiddleButton == mouseEvent->button())
        {
            m_checkForDrag = false;

            if (!isInteractive())
            {
                QMouseEvent customMouseEvent(
                    mouseEvent->type(), mouseEvent->pos(), Qt::LeftButton, Qt::LeftButton, mouseEvent->modifiers());
                QGraphicsView::mouseReleaseEvent(&customMouseEvent);
                mouseEvent->accept();
                setInteractive(true);
                setDragMode(QGraphicsView::RubberBandDrag);
                return;
            }
        }

        if (Qt::LeftButton == mouseEvent->button())
        {
            QGraphicsItem* graphicsItem = this->scene()->mouseGrabberItem();
            if (graphicsItem == nullptr && m_viewDetail != nullptr)
            {
                m_viewDetail->Init(m_busID);
                m_viewDetail->DefaultDisplay();
            }
            else if (graphicsItem != nullptr)
            {
                m_viewDetail->ResetDefaultDisplay();
            }
        }

        QGraphicsView::mouseReleaseEvent(mouseEvent);
    }

    void ParticleGraphicsView::SetItemSelected() const
    {
        for (auto& item : this->scene()->items(sceneRect()))
        {
            QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
            if (itemProxyWidget != nullptr)
            {
                ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
                if (itemWidget != nullptr)
                {
                    itemWidget->OnRelease();
                }
            }
        }
    }

    void ParticleGraphicsView::SetItemZValue() const
    {
        for (auto& item : this->scene()->items(sceneRect()))
        {
            QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
            if (itemProxyWidget != nullptr)
            {
                ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
                if (itemWidget != nullptr)
                {
                    if (itemWidget->m_widgetSelect)
                    {
                        item->setZValue(1);
                    }
                    else
                    {
                        item->setZValue(0);
                    }
                }
            }
        }
    }

    void ParticleGraphicsView::wheelEvent(QWheelEvent* wheelEvent)
    {
        if (!(wheelEvent->modifiers() & Qt::ControlModifier))
        {
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            qreal scaleFactor = 1.0 + (wheelEvent->angleDelta().y() * VIEW_WHEEL_SCALE);
            qreal newScale = m_scale * scaleFactor;

            if (newScale < m_minZoom)
            {
                newScale = m_minZoom;
                scaleFactor = (m_minZoom / m_scale);
            }
            else if (newScale > m_maxZoom)
            {
                newScale = m_maxZoom;
                scaleFactor = (m_maxZoom / m_scale);
            }

            m_scale = newScale;
            scale(scaleFactor, scaleFactor);

            wheelEvent->accept();
            setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        }
    }

    QSize ParticleGraphicsView::sizeHint() const
    {
        return QSize(parentWidget()->rect().width(), parentWidget()->rect().height());
    }

    void ParticleGraphicsView::NewItem(
        OpenParticle::ParticleSourceData::DetailInfo* detail, bool setCurMousePos, const AZStd::string& itemName)
    {
        ParticleItemWidget* itemWidget = new ParticleItemWidget(m_viewDetail, detail, m_SourceData, m_busID);
        itemWidget->GetBtnParticleName()->setText(QString::fromUtf8(detail->m_name.c_str()));
        bool bCheckParticleName = false;
        for (auto& iter : detail->m_modules)
        {
            bool bCheck = false;
            AZStd::string strName = iter.first;
            for (auto& iterModue : iter.second)
            {
                bCheck = iterModue.second.first;
                if (bCheck)
                {
                    bCheckParticleName = true;
                    break;
                }
            }
            SetModulesChecked(strName, bCheck, itemWidget);
        }
        itemWidget->m_ui->checkParticleName->setChecked(bCheckParticleName);
        QGraphicsProxyWidget* item = scene()->addWidget(itemWidget);
        item->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
        AZ::Vector2 vecScene;
        if (setCurMousePos)
        {
            vecScene = MapToScene(QPointToVector(m_initialClick));
            item->setPos(vecScene.GetX(), vecScene.GetY());
        }
        else
        {
            vecScene = MapToScene(QPointToVector(QPoint(0, 0)));
            item->setPos(vecScene.GetX() + itemWidget->width() * m_itemCount, vecScene.GetY());

            if (itemName.size() > 0)
            {
                vecScene = MapToScene(QPointToVector(m_initialClick));
                item->setPos(vecScene.GetX(), vecScene.GetY());
            }
        }
        m_itemCount++;
    }

    void ParticleGraphicsView::AddItem()
    {
        AZ::u8 n = 0;
        const AZ::u8 emittersMax = 100;
        AZStd::string findName = "Emitter";
        if (m_SourceData == nullptr)
        {
            QString message = tr("No particle file is selected! Please select a particle file in the Explore window.");
            QMessageBox messageBox(QMessageBox::Critical, QString(), message, QMessageBox::Ok);
            messageBox.exec();
            return;
        }
        auto& emitters = m_SourceData->m_emitters;
        AZStd::vector<OpenParticle::ParticleSourceData::EmitterInfo*>::const_iterator findIter = emitters.begin();
        while (findIter != emitters.end() && n < emittersMax)
        {
            findName = "Emitter" + AZStd::to_string(++n);
            findIter = AZStd::find_if(
                emitters.begin(), emitters.end(),
                [&findName](const OpenParticle::ParticleSourceData::EmitterInfo* iter)
                {
                    return iter->m_name == findName;
                });
        }
        OpenParticle::ParticleSourceData::DetailInfo* detail = nullptr;
        EBUS_EVENT_ID_RESULT(detail, m_busID, ParticleDocumentRequestBus, AddDetail, findName);
        NewItem(detail, true);
        EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, m_busID);
        EBUS_EVENT_ID(m_busID, ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void ParticleGraphicsView::SetModulesChecked(AZStd::string strName, bool bCheck, ParticleItemWidget* itemWidget)
    {
        bool checked = SetModulesCheckedSingle(strName, bCheck, itemWidget);
        if (!checked)
        {
            SetModulesCheckedMult(strName, bCheck, itemWidget);
        }
    }

    bool ParticleGraphicsView::SetModulesCheckedMult(AZStd::string strName, bool bCheck, ParticleItemWidget* itemWidget)
    {
        bool checked = false;
        if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_SPAWN])
        {
            itemWidget->m_ui->checkSpawn->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_PARTICLES])
        {
            itemWidget->m_ui->checkParticles->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_VELOCITY])
        {
            itemWidget->m_ui->checkVelocity->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_SIZE])
        {
            itemWidget->m_ui->checkSize->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_FORCE])
        {
            itemWidget->m_ui->checkForce->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_EVENT])
        {
            itemWidget->m_ui->checkEvent->setChecked(bCheck);
            checked = true;
        }
        return checked;
    }

    bool ParticleGraphicsView::SetModulesCheckedSingle(AZStd::string strName, bool bCheck, ParticleItemWidget* itemWidget)
    {
        bool checked = false;
        if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_LOCATION])
        {
            itemWidget->m_ui->checkLocation->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_COLOR])
        {
            itemWidget->m_ui->checkColor->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_LIGHT])
        {
            itemWidget->m_ui->checkLight->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_SUBUV])
        {
            itemWidget->m_ui->checkSubUV->setChecked(bCheck);
            checked = true;
        }
        else if (strName == PARTICLE_LINE_NAMES[WIDGET_LINE_RENDERER])
        {
            itemWidget->m_ui->checkRenderer->setChecked(bCheck);
            checked = true;
        }
        return checked;
    }

    void ParticleGraphicsView::AddItemFromData(OpenParticle::ParticleSourceData::DetailInfo* detail)
    {
        NewItem(detail, false);
    }

    void ParticleGraphicsView::DeleteItem()
    {
        QGraphicsItem* graphicsItem = this->scene()->mouseGrabberItem();
        if (graphicsItem == nullptr)
        {
            return;
        }
        QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(graphicsItem);
        ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
        OpenParticle::ParticleSourceData::DetailInfo * detail = itemWidget->m_detail;
        if (detail != nullptr)
        {
            if (m_viewDetail != nullptr)
            {
                if (QString detailName = QString::fromUtf8(detail->m_name.c_str()); detailName == m_viewDetail->m_ui->particleName->text())
                {
                    m_viewDetail->Init(m_busID);
                }
            }
            EBUS_EVENT_ID(m_busID,ParticleDocumentRequestBus, RemoveDetail, detail);
            EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, m_busID);
        }
        this->scene()->removeItem(graphicsItem);
        m_itemCount--;
    }

    void ParticleGraphicsView::CopyItem()
    {
        QGraphicsItem* graphicsItem = this->scene()->mouseGrabberItem();
        if (graphicsItem == nullptr)
        {
            return;
        }

        AZStd::string copyWidgetName = "";
        EBUS_EVENT_RESULT(copyWidgetName, ParticleDocumentRequestBus, GetCopyWidgetName);
        if (!copyWidgetName.empty())
        {
            EBUS_EVENT_ID(copyWidgetName, ParticleDocumentRequestBus, ClearCopyCache);
        }

        QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(graphicsItem);
        ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
        AZStd::string emitterName = itemWidget->m_detail->m_name;
        OpenParticle::ParticleSourceData::DetailInfo* detail = nullptr;
        EBUS_EVENT_ID_RESULT(detail, m_busID, ParticleDocumentRequestBus, CopyDetail, emitterName);
        // tell every document which widget prepare to be pasted
        EBUS_EVENT(ParticleDocumentRequestBus, SetCopyName, m_busID);
    }

    void ParticleGraphicsView::PasteItem()
    {
        OpenParticle::ParticleSourceData::DetailInfo* sourceDetail = nullptr;
        OpenParticle::ParticleSourceData::EmitterInfo* sourceEmitter = nullptr;
        AZStd::string widgetName = "";
        EBUS_EVENT_RESULT(widgetName, ParticleDocumentRequestBus, GetCopyWidgetName);
        if (widgetName.empty())
        {
            AZ_Printf("Emitter", "m_copyWidgetTitleName is null");
            return;
        }

        EBUS_EVENT_ID_RESULT(sourceDetail, widgetName, ParticleDocumentRequestBus, GetDetail);
        EBUS_EVENT_ID_RESULT(sourceEmitter, widgetName, ParticleDocumentRequestBus, GetEmitter);
        OpenParticle::ParticleSourceData::DetailInfo* destDetail = nullptr;
        EBUS_EVENT_ID_RESULT(destDetail, m_busID, ParticleDocumentRequestBus, SetEmitterAndDetail, sourceEmitter, sourceDetail);

        NewItem(destDetail, false, destDetail->m_name);
        
        AZStd::vector<AZStd::string> itemNames = GetItemNamesByOrder();
        for (auto& detailName : itemNames)
        {
            m_SourceData->UpdateDetailSoloState(detailName, false);
            m_SourceData->SelectDetail(detailName);
        }
        AZStd::string nullName;
        SetCheckedSolo(nullName, false, false);
        SetChecked(nullName, false, true);

        EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, m_busID);
        EBUS_EVENT_ID(m_busID, ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    AZ::Vector2 ParticleGraphicsView::GetViewSceneCenter() const
    {
        AZ::Vector2 viewCenter(0, 0);
        QPointF centerPoint = mapToScene(rect()).boundingRect().center();

        viewCenter.SetX(aznumeric_cast<float>(centerPoint.x()));
        viewCenter.SetY(aznumeric_cast<float>(centerPoint.y()));

        return viewCenter;
    }

    AZ::Vector2 ParticleGraphicsView::MapToGlobal(const AZ::Vector2& scenePoint)
    {
        QPoint mapped = mapToGlobal(mapFromScene(AZToQPoint(scenePoint)));
        return QPointToVector(mapped);
    }

    AZ::Vector2 ParticleGraphicsView::MapToScene(const AZ::Vector2& view)
    {
        QPointF mapped = mapToScene(AZToQPoint(view).toPoint());
        return QPointToVector(mapped);
    }

    AZ::Vector2 ParticleGraphicsView::MapFromScene(const AZ::Vector2& scene)
    {
        QPoint mapped = mapFromScene(AZToQPoint(scene));
        return QPointToVector(mapped);
    }

    QPointF ParticleGraphicsView::AZToQPoint(const AZ::Vector2& vector)
    {
        return QPointF(vector.GetX(), vector.GetY());
    }

    AZ::Vector2 ParticleGraphicsView::QPointToVector(const QPointF& point)
    {
        return AZ::Vector2(aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()));
    }
    AZStd::vector<AZStd::string> ParticleGraphicsView::GetItemNamesByOrder()
    {
        AZStd::vector<AZStd::pair<float, AZStd::string>> itemsInfo;

        m_afterSortList.clear();
        SortAllItem(this->scene()->items(sceneRect()));

        for (const auto& item : m_afterSortList)
        {
            QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
            if (itemProxyWidget != nullptr)
            {
                ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
                if (itemWidget != nullptr)
                {
                    auto pos = item->scenePos();
                    auto info = AZStd::make_pair(static_cast<float>(pos.x()), itemWidget->m_detail->m_name);
                    itemsInfo.push_back(info);
                }
            }
        }

        AZStd::vector<AZStd::string> result;
        for (const auto& item : itemsInfo)
        {
            result.push_back(item.second);
        }
        return result;
    }

    void ParticleGraphicsView::SetCheckedInternal(bool solo, AZStd::string& name, bool except, bool checked)
    {
        auto SetCheckedFun = [](ParticleItemWidget* item, bool solo, bool checked)
        {
            if (solo)
            {
                item->SetSoloChecked(checked);
            }
            else
            {
                item->SetDetailChecked(checked);
            }
        };

        size_t len = name.length();
        for (const auto& item : this->scene()->items(sceneRect()))
        {
            QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
            if (itemProxyWidget == nullptr) {
                continue;
            }
            ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
            if (itemWidget == nullptr) {
                continue;
            }
            if (except)
            {
                if (name.compare(itemWidget->m_detail->m_name) == 0)
                {
                    continue;
                }
                AZStd::invoke(SetCheckedFun, itemWidget, solo, checked);
            }
            else
            {
                if (len == 0 || name.compare(itemWidget->m_detail->m_name) == 0)
                {
                    AZStd::invoke(SetCheckedFun, itemWidget, solo, checked);
                    if (len != 0)
                    {
                        break;
                    }
                }
            }
        }
    }

    void ParticleGraphicsView::CheckAllParticleItemWidgetWithoutBusNotification()
    {
        auto emitterWidgets = GetAllParticleItemWidgets();
        auto emitterNames = GetItemNamesByOrder();
        for (auto& emitterName : emitterNames)
        {
            // save check status
            m_itemWidgetsCheckStatus[emitterName] = emitterWidgets[emitterName]->m_detail->m_isUse;
            if (!m_itemWidgetsCheckStatus[emitterName])
            {
                // set widget to be checked
                emitterWidgets[emitterName]->m_pSourceData->SelectDetail(emitterName);
                emitterWidgets[emitterName]->m_pSourceData->SortEmitters(emitterNames);
            }
        }
        m_SourceData->SortEmitters(emitterNames);
    }

    void ParticleGraphicsView::RestoreAllParticleItemWidgetStatusAfterCheckAll()
    {
        auto emitterWidgets = GetAllParticleItemWidgets();
        auto emitterNames = GetItemNamesByOrder();
        for (auto& emitterName : emitterNames)
        {
            // restore check status
            if (!m_itemWidgetsCheckStatus[emitterName])
            {
                // set widget to be unchecked
                emitterWidgets[emitterName]->m_pSourceData->UnselectDetail(emitterName);
            }
        }
        SaveAllViewItemPos();
    }

    AZStd::unordered_map<AZStd::string, ParticleItemWidget*> ParticleGraphicsView::GetAllParticleItemWidgets()
    {
        AZStd::unordered_map<AZStd::string, ParticleItemWidget*> widgets;
        for (const auto& item : this->scene()->items(sceneRect()))
        {
            QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
            if (itemProxyWidget != nullptr)
            {
                ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
                if (itemWidget != nullptr)
                {
                    widgets[itemWidget->m_detail->m_name] = itemWidget;
                }
            }
        }
        return widgets;
    }

    void ParticleGraphicsView::SetCheckedSolo(AZStd::string& name, bool except, bool checked)
    {
        SetCheckedInternal(true, name, except, checked);
    }

    void ParticleGraphicsView::SetChecked(AZStd::string& name, bool except, bool checked)
    {
        SetCheckedInternal(false, name, except, checked);
    }
    bool ParticleGraphicsView::PointOnItem(QPoint pt)
    {
        QPointF scenePt = this->mapToScene(pt);
        for (const auto& item : this->scene()->items(sceneRect()))
        {
            QGraphicsProxyWidget* itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
            if (itemProxyWidget != nullptr)
            {
                ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
                if (itemWidget != nullptr)
                {
                    auto pos = item->scenePos();
                    auto sz = itemWidget->size();
                    auto name = itemWidget->m_detail->m_name;

                    QRectF rect(pos.x(), pos.y(), sz.width(), sz.height());
                    if (rect.contains(scenePt))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    void ParticleGraphicsView::PositionAllItems()
    {
        auto itemList = scene()->items(sceneRect());
        AZStd::sort(itemList.begin(), itemList.end(), [](const auto& first, const auto& second)
            {
                QGraphicsProxyWidget* firstProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(first);
                QGraphicsProxyWidget* secondProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(second);
                return firstProxyWidget->scenePos().x() < secondProxyWidget->scenePos().x();
            });
        AZ::Vector2 vecScene = MapToScene(QPointToVector(QPoint(0, 0)));
        AZ::u32 index = 0;
        QGraphicsProxyWidget* itemProxyWidget = nullptr;
        ParticleItemWidget* itemWidget = nullptr;
        for (const auto& item : itemList)
        {
            itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
            itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
            item->setPos(vecScene.GetX() + index * itemWidget->width(), vecScene.GetY());
            index++;
        }
    }

    void ParticleGraphicsView::SaveAllViewItemPos()
    {
        auto itemList = scene()->items(sceneRect());
        m_afterSortList.clear();
        SortAllItem(itemList);

        AZ::s32 itemX = 0;
        AZ::s32 itemY = 0;
        QFile particleLayoutSettings(m_particleAssetPath);
        if (!particleLayoutSettings.open(QIODevice::WriteOnly))
        {
            AZ_Printf("Emitter", "Emitter position save failed:%s", particleLayoutSettings.errorString().toStdString().c_str());
            return;
        }
        QDataStream saveInFile(&particleLayoutSettings);
        for (const auto& item : m_afterSortList)
        {
            itemX = item->x();
            itemY = item->y();
            saveInFile << itemX;
            saveInFile << itemY;
        }
        particleLayoutSettings.close();
    }

    void ParticleGraphicsView::SortAllItem(QList<QGraphicsItem*> remainItemList)
    {
        if (remainItemList.empty())
        {
            return;
        }
        QGraphicsProxyWidget* itemProxyWidget = nullptr;
        ParticleItemWidget* itemWidget = nullptr;
        itemProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(remainItemList[0]);
        itemWidget = dynamic_cast<ParticleItemWidget*>(itemProxyWidget->widget());
        AZ::s32 itemHeight = itemWidget->height();

        QGraphicsItem* highestItem = remainItemList[0];
        QList<QGraphicsItem*> oneFloorList;
        QList<QGraphicsItem*> nextRemainItemList;        

        for (const auto& item : remainItemList)
        {
            AZ::s32 highestItemY = highestItem->y();
            AZ::s32 remainItemY = item->y();
            if (highestItemY > remainItemY)
            {
                highestItem = item;
            }
        }

        for (const auto& item : remainItemList)
        {
            AZ::s32 highestItemY = highestItem->y();
            AZ::s32 remainItemY = item->y();
            if (highestItemY <= remainItemY && highestItemY + itemHeight > remainItemY)
            {
                oneFloorList.append(item);
            }
            else
            {
                nextRemainItemList.append(item);
            }
        }

        AZStd::sort(oneFloorList.begin(), oneFloorList.end(), [](const auto& first, const auto& second)
        {
            QGraphicsProxyWidget* firstProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(first);
            QGraphicsProxyWidget* secondProxyWidget = dynamic_cast<QGraphicsProxyWidget*>(second);
            return firstProxyWidget->scenePos().x() < secondProxyWidget->scenePos().x();
        });
                
        for (const auto& item : oneFloorList)
        {
            m_afterSortList.append(item);
        }
        SortAllItem(nextRemainItemList);
    }

    void ParticleGraphicsView::ReadAllViewItemPos(AZStd::string particleAssetPath)
    {
        m_particleAssetPath = QString::fromStdString(particleAssetPath.c_str());
        m_particleAssetPath.append("layout");
        auto itemList = scene()->items(sceneRect());
        m_afterSortList.clear();
        SortAllItem(itemList);

        AZ::s32 itemX = 0;
        AZ::s32 itemY = 0;
        QFile particleLayoutSettings(m_particleAssetPath);
        if (particleLayoutSettings.exists() && !particleLayoutSettings.open(QIODevice::ReadOnly))
        {
            AZ_Printf("Emitter", "Emitter position read failed:%s", particleLayoutSettings.errorString().toStdString().c_str());
            PositionAllItems();
            return;
        }
        QDataStream readFromFile(&particleLayoutSettings);
        for (const auto& item : m_afterSortList)
        {
            readFromFile >> itemX;
            readFromFile >> itemY;
            item->setPos(itemX, itemY);
        }
        particleLayoutSettings.close();
        ResetCenter();
    }

    void ParticleGraphicsView::ResetCenter()
    {
        auto itemList = scene()->items(sceneRect());
        AZ::s32 viewX = 0;
        AZ::s32 viewY = 0;
        AZ::s32 index = 0;
        for (const auto& item : itemList)
        {
            if (index == 0)
            {
                viewX = item->x();
                viewY = item->y();
                index++;
            }
            else
            {
                if (item->x() < viewX)
                {
                    viewX = item->x();
                }
                if (item->y() < viewY)
                {
                    viewY = item->y();
                }
            }
        }
        centerOn(viewX + (this->width() / 2), viewY + (this->height() / 2));
    }

    void ParticleGraphicsView::ShowMenu(const QPoint& pos)
    {
        QMenu* menu = new QMenu(this);
        // in order "Add,Copy,Paste,Delete"
        menu->addAction(m_pActAddItem);
        if (PointOnItem(pos))
        {
            menu->addAction(m_pActDelItem);
        }

        menu->move(cursor().pos());
        if (this->scene()->selectedItems().isEmpty())
        {
            this->m_pActDelItem->setEnabled(true);
            this->m_pActCopyItem->setEnabled(true);
            AZStd::string widgetName = "";
            EBUS_EVENT_RESULT(widgetName, ParticleDocumentRequestBus, GetCopyWidgetName);
            if (!widgetName.empty())
            {
                this->m_pActPasteItem->setEnabled(true);
            }
        }
        else
        {
            this->m_pActAddItem->setEnabled(true);
        }
        menu->show();
    }
} // namespace OpenParticleSystemEditor
