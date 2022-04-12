/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/stack.h>
#include <QVector>
#include <Editor/ObjectEditorCard.h>

QT_FORWARD_DECLARE_CLASS(QWidget);

namespace EMStudio
{
    class ObjectEditorCardPool
    {
    public:
        ObjectEditorCard* GetFree(AZ::SerializeContext* serializeContext, QWidget* parent);
        void ReturnAllCards();

    private:
        AZStd::stack<AZStd::unique_ptr<ObjectEditorCard>> m_availableRpeCards;
        QVector<ObjectEditorCard*> m_usedRpeCards;
    };
} // namespace EMStudio
