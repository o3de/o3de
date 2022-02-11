/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/ReflectContext.h>

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#include <QList>
#endif

class QGridLayout;
class QPushButton;
class QButtonGroup;
class QStringList;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ButtonStripe
        : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ButtonStripe, AZ::SystemAllocator, 0);
        AZ_RTTI(ButtonStripe, "{A2551A0B-FB22-46ED-9485-3FC8A22E31AD}");

        explicit ButtonStripe(QWidget* parent = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        void addButtons(const QStringList& buttonNames, int current = 0);
        void setCurrent(int index);

    signals:
        void buttonClicked(int);

    private:
        QGridLayout* const m_gridLayout;
        QButtonGroup* const m_buttonGroup;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QList<QPushButton*> m_buttons;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
} // namespace AzQtComponents

