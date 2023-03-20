/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCardCollection.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCard.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzCore/Casting/numeric_cast.h>
#include <QVBoxLayout>

namespace AzQtComponents
{

    PaletteCardCollection::PaletteCardCollection(Internal::ColorController* colorController, QUndoStack* undoStack, QWidget* parent)
        : QWidget(parent)
        , m_colorController(colorController)
        , m_undoStack(undoStack)
        , m_layout(new QVBoxLayout(this))
        , m_swatchSize({ 16, 16 })
    {
        m_layout->setContentsMargins(0, 0, 0, 0);
    }

    PaletteCardCollection::~PaletteCardCollection()
    {
        for (QSharedPointer<PaletteCard> card : m_paletteCards)
        {
            m_layout->removeWidget(card.data());
            card->setParent(nullptr);
        }
    }

    QSharedPointer<PaletteCard> PaletteCardCollection::makeCard(QSharedPointer<Palette> palette, const QString& title)
    {
        auto card = QSharedPointer<PaletteCard>::create(palette, m_colorController, m_undoStack, this);
        card->setTitle(uniquePaletteName(card, title));
        card->setSwatchSize(m_swatchSize);
        card->setGammaEnabled(m_gammaEnabled);
        card->setGamma(m_gamma);
        card->setMaximumWidth(m_paletteWidth);

        return card;
    }

    void PaletteCardCollection::addCard(QSharedPointer<PaletteCard> card)
    {
        // The signals below are connected to lambdas, we can't use Qt::UniqueConnection
        // so we use m_registeredPaletteCards to track them instead, because otherwise,
        // we get the same lambda called for every single time it was connected,
        // which with undo/redo, can be more than you ever want it to.
        if (m_registeredPaletteCards.find(card.data()) == m_registeredPaletteCards.end())
        {
            connect(card.data(), &PaletteCard::removeClicked, this, [this, card] {
                emit removePaletteClicked(card);
            });
            connect(card.data(), &PaletteCard::saveClicked, this, [this, card] {
                emit savePaletteClicked(card);
            });
            connect(card.data(), &PaletteCard::saveAsClicked, this, [this, card] {
                emit savePaletteAsClicked(card);
            });

            connect(card.data(), &QObject::destroyed, this, &PaletteCardCollection::paletteCardDestroyed);

            m_registeredPaletteCards.insert(card.data());
        }

        m_layout->addWidget(card.data());
        m_paletteCards.append(card);

        emit paletteCountChanged();
    }

    void PaletteCardCollection::removeCard(QSharedPointer<PaletteCard> card)
    {
        auto target = std::find(m_paletteCards.begin(), m_paletteCards.end(), card);

        if (target != m_paletteCards.end())
        {
            m_paletteCards.erase(target);
            m_layout->removeWidget(card.data());
            card->setParent(nullptr);
        }

        emit paletteCountChanged();
    }

    bool PaletteCardCollection::containsCard(QSharedPointer<PaletteCard> card) const
    {
        return m_paletteCards.contains(card);
    }

    void PaletteCardCollection::setSwatchSize(const QSize& size)
    {
        if (size == m_swatchSize)
        {
            return;
        }

        m_swatchSize = size;

        for (auto card : m_paletteCards)
        {
            card->setSwatchSize(size);
        }
    }

    void PaletteCardCollection::setGammaEnabled(bool enabled)
    {
        if (enabled == m_gammaEnabled)
        {
            return;
        }

        m_gammaEnabled = enabled;

        for (auto card : m_paletteCards)
        {
            card->setGammaEnabled(enabled);
        }
    }

    void PaletteCardCollection::setGamma(qreal gamma)
    {
        if (qFuzzyCompare(gamma, m_gamma))
        {
            return;
        }

        m_gamma = gamma;

        for (auto card : m_paletteCards)
        {
            card->setGamma(gamma);
        }
    }

    int PaletteCardCollection::count() const
    {
        return m_paletteCards.count();
    }

    bool PaletteCardCollection::isEmpty() const
    {
        return m_paletteCards.isEmpty();
    }

    QSharedPointer<PaletteCard> PaletteCardCollection::paletteCard(int index) const
    {
        return m_paletteCards[index];
    }

    int PaletteCardCollection::indexOf(const QSharedPointer<PaletteCard>& card) const
    {
        return m_paletteCards.indexOf(card);
    }

    void PaletteCardCollection::moveUp(QSharedPointer<PaletteCard>& card)
    {
        int index = indexOf(card);
        if (index > 0)
        {
            m_paletteCards.remove(index);
            m_paletteCards.insert(index - 1, card);

            m_layout->removeWidget(card.data());
            m_layout->insertWidget(index - 1, card.data());
        }
    }

    bool PaletteCardCollection::canMoveUp(QSharedPointer<PaletteCard>& card)
    {
        int index = indexOf(card);
        return (index > 0);
    }

    void PaletteCardCollection::moveDown(QSharedPointer<PaletteCard>& card)
    {
        int index = indexOf(card);
        if (index < (m_paletteCards.count() - 1))
        {
            m_paletteCards.remove(index);
            m_paletteCards.insert(index + 1, card);

            m_layout->removeWidget(card.data());
            m_layout->insertWidget(index + 1, card.data());
        }
    }

    bool PaletteCardCollection::canMoveDown(QSharedPointer<PaletteCard>& card)
    {
        int index = indexOf(card);
        return (index < (m_paletteCards.count() - 1));
    }

    void PaletteCardCollection::setCardContentMargins(const QMargins& margins)
    {
        for (auto card : m_paletteCards)
        {
            card->setContentsMargins(margins);
        }
    }

    QString PaletteCardCollection::uniquePaletteName(QSharedPointer<PaletteCard> card, const QString& name) const
    {
        const auto paletteNameExists = [this](const QString& name)
        {
            auto it = std::find_if(m_paletteCards.begin(), m_paletteCards.end(),
                [&name](QSharedPointer<const PaletteCard> card)
            { return name == card->title(); });
            return it != m_paletteCards.end();
        };

        if (!paletteNameExists(name))
        {
            return name;
        }

        auto lastDigit = std::find_if(name.rbegin(), name.rend(),
            [](const QChar ch) { return !ch.isDigit(); });
        const QString baseName = name.left(aznumeric_cast<int>(std::distance(name.begin(), lastDigit.base())));

        int lastNumber = 1;
        while (true)
        {
            QString newName = QStringLiteral("%1%2").arg(baseName).arg(lastNumber);
            if (!paletteNameExists(newName))
            {
                return newName;
            }
            ++lastNumber;
        }

        Q_UNREACHABLE();
    }

    void PaletteCardCollection::paletteCardDestroyed(QObject* obj)
    {
        m_registeredPaletteCards.remove(obj);
    }

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_PaletteCardCollection.cpp"
