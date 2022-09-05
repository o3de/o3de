/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <QWidget>
#include <QItemSelection>
#endif


QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class Node;

    class SkeletonModelJointWidget
        : public QWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        SkeletonModelJointWidget(QWidget* parent = nullptr);
        ~SkeletonModelJointWidget() = default;

        virtual void CreateGUI();

        void Reinit();

        void showEvent(QShowEvent* event) override;

        virtual QString GetCardTitle() const = 0;
        virtual QColor GetColor() const = 0;
    protected:
        Actor* GetActor() const;
        ActorInstance* GetActorInstance();
        Node* GetNode() const;
        QModelIndexList GetSelectedModelIndices() const;
        virtual QWidget* CreateContentWidget(QWidget* parent) = 0;
        virtual QWidget* CreateNoSelectionWidget(QWidget* parent) = 0;
        virtual void InternalReinit() = 0;

    public slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
        void OnModelReset();

    protected:
        AzQtComponents::Card*   m_contentCard;

        QLabel*         m_jointNameLabel;
        static int      s_jointLabelSpacing;
        static int      s_jointNameSpacing;

    private:
        QWidget*        m_noSelectionWidget;
    };
} // namespace EMotionFX
