/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <OpenParticleSystem/ParticleGraphicsViewRequestsBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <EffectorInspector.h>
#include <Window/ParticleItemWidget.h>

#if !defined(Q_MOC_RUN)
#include <QGraphicsView>
#include <QMenu>
#include <Serializer/ParticleSourceData.h>
#endif

namespace OpenParticleSystemEditor
{
    class ParticleGraphicsView
        : public QGraphicsView
        , public ParticleGraphicsViewRequestsBus::Handler
    {
        Q_OBJECT
    public:
        ParticleGraphicsView(QWidget* parent = nullptr);
        virtual ~ParticleGraphicsView();
        void SetModulesChecked(AZStd::string strName, bool bCheck, ParticleItemWidget* itemWidget);
        void AddItemFromData(OpenParticle::ParticleSourceData::DetailInfo* detail);
        void SetItemSelected() const;
        void SetItemZValue() const;
        void NewItem(OpenParticle::ParticleSourceData::DetailInfo* detail, bool setCurMousePos, const AZStd::string& itemName = "");
        void PositionAllItems();
        AZStd::vector<AZStd::string> GetItemNamesByOrder() override;
        void SetCheckedSolo(AZStd::string& name, bool except, bool checked) override;
        void SetChecked(AZStd::string& name, bool except, bool checked) override;
        void CheckAllParticleItemWidgetWithoutBusNotification() ;
        void RestoreAllParticleItemWidgetStatusAfterCheckAll() ;
        AZStd::unordered_map<AZStd::string, ParticleItemWidget*> GetAllParticleItemWidgets();

        EffectorInspector* m_viewDetail = nullptr;
        OpenParticle::ParticleSourceData* m_SourceData = nullptr;
        unsigned m_itemCount = 0;

        void SaveAllViewItemPos();
        void ReadAllViewItemPos(AZStd::string particleAssetPath);
        void SortAllItem(QList<QGraphicsItem*> remainItemList);
        void SetEBusId(QString windowTitle);

    protected:
        void SetupUi();
        void contextMenuEvent(QContextMenuEvent* contextMenuEvent) override;
        void mousePressEvent(QMouseEvent* mouseEvent) override;
        void mouseMoveEvent(QMouseEvent* mouseEvent) override;
        void mouseReleaseEvent(QMouseEvent* mouseEvent) override;
        void wheelEvent(QWheelEvent* wheelEvent) override;
        QSize sizeHint() const override;

        AZ::Vector2 GetViewSceneCenter() const;
        AZ::Vector2 MapToGlobal(const AZ::Vector2& scenePoint);
        AZ::Vector2 MapToScene(const AZ::Vector2& view);
        AZ::Vector2 MapFromScene(const AZ::Vector2& scene);

        QPointF AZToQPoint(const AZ::Vector2& vector);
        AZ::Vector2 QPointToVector(const QPointF& point);

        void SetCheckedInternal(bool solo, AZStd::string& name, bool except, bool checked);
        bool PointOnItem(QPoint pt);
        void ShowMenu(const QPoint& pos);
        void ResetCenter();

    private slots:
        void AddItem();
        void DeleteItem();
        void CopyItem();
        void PasteItem();

    private:

        bool SetModulesCheckedMult(AZStd::string strName, bool bCheck, ParticleItemWidget* itemWidget);
        bool SetModulesCheckedSingle(AZStd::string strName, bool bCheck, ParticleItemWidget* itemWidget);

        bool m_checkForEdges;
        bool m_checkForDrag;
        bool m_checkForMove;
        QPoint m_initialClick;

        QAction* m_pActAddItem = nullptr;
        QAction* m_pActDelItem = nullptr;
        QAction* m_pActCopyItem = nullptr;
        QAction* m_pActPasteItem = nullptr;
                
        QString m_particleAssetPath = "";

        double m_scale;
        float m_anchorPointX;
        float m_anchorPointY;
        qreal m_minZoom;
        qreal m_maxZoom;

        AZStd::string m_busID;
        QList<QGraphicsItem*> m_afterSortList;

        // <emmiter widget name, check value> only used for func pair: 
        // CheckAllParticleItemWidgetWithoutBusNotification
        // RestoreAllParticleItemWidgetStatusAfterCheckAll
        AZStd::unordered_map<AZStd::string, bool> m_itemWidgetsCheckStatus;
    };
} // namespace OpenParticleSystemEditor
