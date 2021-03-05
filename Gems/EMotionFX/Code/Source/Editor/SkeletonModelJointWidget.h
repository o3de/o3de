/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        void Reinit(const QModelIndexList& selectedModelIndices);

        void showEvent(QShowEvent* event) override;

    protected:
        Actor* GetActor() const;
        Node* GetNode() const;
        virtual QWidget* CreateContentWidget(QWidget* parent) = 0;
        virtual QWidget* CreateNoSelectionWidget(QWidget* parent) = 0;
        virtual void InternalReinit() = 0;

    public slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
        void OnModelReset();

    protected:
        QModelIndexList m_selectedModelIndices;
        QLabel*         m_jointNameLabel;
        static int      s_jointLabelSpacing;
        static int      s_jointNameSpacing;

    private:
        QWidget*        m_contentsWidget;
        QWidget*        m_noSelectionWidget;
    };
} // namespace EMotionFX
