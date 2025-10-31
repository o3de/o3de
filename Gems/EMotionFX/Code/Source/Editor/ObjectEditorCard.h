/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/Components/Widgets/Card.h>
#include <Source/Editor/ObjectEditor.h>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace AZ
{
    class SerializeContext;
}

namespace EMStudio
{
    class ObjectEditorCard
        : public AzQtComponents::Card
    {
        Q_OBJECT //AUTOMOC

    public:
        ObjectEditorCard(AZ::SerializeContext* serializeContext, QWidget* parent = nullptr);
        ~ObjectEditorCard() override = default;

        void Update(const QString& cardName, const AZ::TypeId& objectTypeId, void* object);

        EMotionFX::ObjectEditor* GetObjectEditor() const { return m_objectEditor; }

    private:
        EMotionFX::ObjectEditor* m_objectEditor = nullptr;
    };
} // namespace EMStudio
