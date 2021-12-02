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
#include <QWidget>
#include <QItemSelection>
#endif


QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMotionFX
{
    class Actor;
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

    protected:
        Actor* GetActor() const;
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
        QLabel*         m_jointNameLabel;
        static int      s_jointLabelSpacing;
        static int      s_jointNameSpacing;

    private:
        QWidget*        m_contentsWidget;
        QWidget*        m_noSelectionWidget;
    };
} // namespace EMotionFX
