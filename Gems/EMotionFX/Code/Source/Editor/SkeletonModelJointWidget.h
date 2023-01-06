/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/TypeInfo.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <QItemSelection>
#include <QVBoxLayout>
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class Node;
    class ColliderContainerWidget;

    class SkeletonModelJointWidget : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public : SkeletonModelJointWidget(QWidget* parent = nullptr);
        ~SkeletonModelJointWidget() = default;

        virtual void CreateGUI();

        void Reinit();

        void showEvent(QShowEvent* event) override;

        void SetFilterString(QString filterString);

        virtual QString GetCardTitle() const = 0;
        virtual QColor GetColor() const = 0;

        void ErrorNotification(QString title, QString description);

    protected:
        Actor* GetActor() const;
        ActorInstance* GetActorInstance();
        Node* GetNode() const;
        QModelIndexList GetSelectedModelIndices() const;
        virtual QWidget* CreateContentWidget(QWidget* parent) = 0;
        virtual void InternalReinit() = 0;
        virtual int WidgetCount() const = 0;

    signals:
        void WidgetCountChanged();
    public slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
        void OnModelReset();

    protected:
        QWidget* m_content = nullptr;
        AzQtComponents::Card* m_contentCard = nullptr;
        ColliderContainerWidget* m_collidersWidget = nullptr;

        QLabel* m_jointNameLabel;
        static int s_jointLabelSpacing;
        static int s_jointNameSpacing;
    };
} // namespace EMotionFX
