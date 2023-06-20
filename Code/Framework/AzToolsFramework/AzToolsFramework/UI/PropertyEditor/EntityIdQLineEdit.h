/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string_view.h>

#include <QLineEdit>

namespace AzToolsFramework
{
    class EntityIdQLineEdit
        : public QLineEdit
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR(EntityIdQLineEdit, AZ::SystemAllocator);

        explicit EntityIdQLineEdit(QWidget* parent = nullptr);
        ~EntityIdQLineEdit() override;

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
