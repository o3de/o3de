/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ComboBoxEditButtonPair.h>

namespace NvCloth
{
    namespace Editor
    {
        ComboBoxEditButtonPair::ComboBoxEditButtonPair(QWidget* parent)
            : QWidget(parent)
        {
            QHBoxLayout* layout = new QHBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);

            m_comboBox = new QComboBox();
            m_comboBox->installEventFilter(this);

            m_editButton = new QToolButton();
            m_editButton->setAutoRaise(true);
            m_editButton->setToolTip(QString("Edit"));
            m_editButton->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));

            layout->addWidget(m_comboBox);
            layout->addWidget(m_editButton);
        }

        bool ComboBoxEditButtonPair::eventFilter(QObject *object, QEvent *event)
        {
            AZ_UNUSED(object);
            return event->type() == QEvent::Wheel;
        }

        QComboBox* ComboBoxEditButtonPair::GetComboBox()
        { 
            return m_comboBox; 
        }

        QToolButton* ComboBoxEditButtonPair::GetEditButton()
        { 
            return m_editButton; 
        }

        void ComboBoxEditButtonPair::SetEntityId(AZ::EntityId entityId)
        {
            m_entityId = entityId;
        }

        AZ::EntityId ComboBoxEditButtonPair::GetEntityId() const
        {
            return m_entityId;
        }

    } // namespace Editor
} // namespace NvCloth
