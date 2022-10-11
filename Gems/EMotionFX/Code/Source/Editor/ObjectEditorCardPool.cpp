/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ObjectEditorCardPool.h>
#include <Editor/ObjectEditorCard.h>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>

namespace EMStudio
{
    ObjectEditorCard* ObjectEditorCardPool::GetFree(AZ::SerializeContext* serializeContext, QWidget* parent)
    {
        ObjectEditorCard* rpeCard = nullptr;
        if (m_availableRpeCards.empty())
        {
            rpeCard = new ObjectEditorCard(serializeContext, parent);
        }
        else
        {
            rpeCard = m_availableRpeCards.top().release();
            m_availableRpeCards.pop();
            rpeCard->setParent(parent);
            rpeCard->show();
        }

        m_usedRpeCards.push_back(rpeCard);
        return rpeCard;
    }

    void ObjectEditorCardPool::ReturnAllCards()
    {
        for (ObjectEditorCard* rpeCard : m_usedRpeCards)
        {
            rpeCard->hide();
            rpeCard->setParent(nullptr);
            m_availableRpeCards.push(AZStd::unique_ptr<ObjectEditorCard>(rpeCard));
        }
        m_usedRpeCards.clear();
    }
} // namespace EMStudio
