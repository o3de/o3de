/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef ENTITY_ID_QLABEL_HXX
#define ENTITY_ID_QLABEL_HXX

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string_view.h>

#include <QtWidgets/QLabel>
#endif

#pragma once

class QSpinBox;
class QPushButton;

namespace AzToolsFramework
{
    class EntityIdQLabel
        : public QLabel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EntityIdQLabel, AZ::SystemAllocator);

        explicit EntityIdQLabel(QWidget* parent = 0);
        ~EntityIdQLabel() override;

        void SetEntityId(AZ::EntityId newId, const AZStd::string_view& nameOverride);
        AZ::EntityId GetEntityId() const { return m_entityId; }

    signals:
        void RequestPickObject();

    protected:
        void mousePressEvent(QMouseEvent* e) override;
        void mouseDoubleClickEvent(QMouseEvent* e) override;

    private:
        AZ::EntityId m_entityId;
    };
}

#endif
