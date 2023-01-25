/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QEvent>
#include <QToolButton>

namespace AzToolsFramework
{
    //! Wrapper widget for a combo box with a button at the end.
    class ComboBoxEditButtonPair : public QWidget
    {
        Q_OBJECT

    public:
        explicit ComboBoxEditButtonPair(QWidget* parent);

        QComboBox* GetComboBox();
        QToolButton* GetEditButton();

        void SetEntityId(AZ::EntityId entityId);
        AZ::EntityId GetEntityId() const;
        int value() const;

     signals:
        void valueChanged(int newValue);

    public slots:
        void setValue(int val);

    protected slots:
        void onChildComboBoxValueChange(int value);

    private:
        bool eventFilter(QObject* object, QEvent* event) override;

        QComboBox* m_comboBox = nullptr;
        QToolButton* m_editButton = nullptr;

        AZ::EntityId m_entityId;
    };
} // namespace AzToolsFramework
