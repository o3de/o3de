/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <QScrollArea>
#include <QItemSelection>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace AzQtComponents
{
    class Card;
} // namespace AzQtComponents

namespace EMotionFX
{
    class Actor;
    class Node;
    class ObjectEditor;
    class AddToSimulatedObjectButton;
    class SimulatedObjectWidget;
    class ObjectEditor;
    class SimulatedObjectPropertyNotify;
    class SimulatedObjectColliderWidget;
    class NotificationWidget;

    class SimulatedJointWidget
        : public QScrollArea
    {
        Q_OBJECT //AUTOMOC

    public:
        explicit SimulatedJointWidget(SimulatedObjectWidget* plugin, QWidget* parent = nullptr);
        ~SimulatedJointWidget() override;

        void UpdateDetailsView(const QItemSelection& selected, const QItemSelection& deselected);

        void RemoveSelectedSimulatedJoint() const;
        void BackToSimulatedObject();

    private slots:
        void OnSkeletonOutlinerSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        void UpdateObjectNotification();

        static const int s_jointLabelSpacing;
        static const int s_jointNameSpacing;

        SimulatedObjectWidget* m_plugin;
        QWidget* m_contentsWidget;
        QPushButton* m_removeButton;
        QPushButton* m_backButton;
        ObjectEditor* m_simulatedObjectEditor = nullptr;
        ObjectEditor* m_simulatedJointEditor = nullptr;
        AzQtComponents::Card* m_simulatedObjectEditorCard;
        AzQtComponents::Card* m_simulatedJointEditorCard;

        // Simulated Joint/Object name label
        QLabel* m_nameLeftLabel = nullptr;
        QLabel* m_nameRightLabel = nullptr;

        NotificationWidget* m_simulatedObjectNotification1 = nullptr;
        NotificationWidget* m_simulatedObjectNotification2 = nullptr;

        AZStd::unique_ptr<SimulatedObjectPropertyNotify> m_propertyNotify;
        QWidget* m_colliderWidget = nullptr;
    };
} // namespace EMotionFX
