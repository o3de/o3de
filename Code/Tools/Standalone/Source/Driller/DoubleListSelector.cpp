/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "DoubleListSelector.hxx"
#include <Source/Driller/moc_DoubleListSelector.cpp>
#include <Source/Driller/ui_DoubleListSelector.h>

namespace Driller
{
    ///////////////////////
    // DoubleListSelector
    ///////////////////////

    DoubleListSelector::DoubleListSelector(QWidget* parent)
        : QWidget(parent)
        , m_gui(new Ui::DoubleListSelector())
    {
        m_gui->setupUi(this);

        m_gui->activateButton->setAutoDefault(false);
        m_gui->deactivateButton->setAutoDefault(false);

        QObject::connect(m_gui->activateButton, SIGNAL(clicked()), this, SLOT(activateSelected()));
        QObject::connect(m_gui->deactivateButton, SIGNAL(clicked()), this, SLOT(deactivateSelected()));
    }

    DoubleListSelector::~DoubleListSelector()
    {
        delete m_gui;
    }

    void DoubleListSelector::setItemList(const QStringList& items, bool maintainActiveList)
    {
        if (maintainActiveList)
        {
            m_gui->inactiveList->clearItems();

            const QStringList& activeItems = m_gui->activeList->getAllItems();

            QStringList newActiveItems;
            QStringList inactiveItems;

            for (const QString& currentItem : items)
            {
                if (activeItems.contains(currentItem))
                {
                    newActiveItems.push_back(currentItem);
                }
                else
                {
                    inactiveItems.push_back(currentItem);
                }
            }

            m_gui->activeList->clearItems();

            m_gui->inactiveList->addItems(inactiveItems);
            m_gui->activeList->addItems(newActiveItems);

            emit ActiveItemsChanged();
        }
        else
        {
            m_gui->inactiveList->clearItems();
            m_gui->inactiveList->addItems(items);
            m_gui->activeList->clearItems();
        }
    }

    void DoubleListSelector::setActiveItems(const QStringList& items)
    {
        QStringList inactiveItems = m_gui->inactiveList->getAllItems();
        const QStringList& activeItems = m_gui->activeList->getAllItems();

        inactiveItems.append(activeItems);

        m_gui->inactiveList->clearItems();
        m_gui->activeList->clearItems();

        for (const QString& currentItem : items)
        {
            QStringList::iterator itemIter = inactiveItems.begin();

            while (itemIter != inactiveItems.end())
            {
                if ((*itemIter).compare(currentItem) == 0)
                {
                    inactiveItems.erase(itemIter);
                    break;
                }

                ++itemIter;
            }
        }

        m_gui->inactiveList->addItems(inactiveItems);
        m_gui->activeList->addItems(items);

        emit ActiveItemsChanged();
    }

    const QStringList& DoubleListSelector::getActiveItems() const
    {
        return m_gui->activeList->getAllItems();
    }

    void DoubleListSelector::setActiveTitle(const QString& title)
    {
        m_gui->activeGroupBox->setTitle(title);
    }

    void DoubleListSelector::setInactiveTitle(const QString& title)
    {
        m_gui->inactiveGroupBox->setTitle(title);
    }

    void DoubleListSelector::activateSelected()
    {
        QStringList activateItems;

        m_gui->inactiveList->getSelectedItems(activateItems);
        m_gui->inactiveList->removeSelected();

        m_gui->activeList->addItems(activateItems);

        emit ActiveItemsChanged();
    }

    void DoubleListSelector::deactivateSelected()
    {
        QStringList activateItems;

        m_gui->activeList->getSelectedItems(activateItems);
        m_gui->activeList->removeSelected();

        m_gui->inactiveList->addItems(activateItems);

        emit ActiveItemsChanged();
    }
}
