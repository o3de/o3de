/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/numeric.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtMath>
#include <QToolButton>
#include <QToolBar>
#include <QSettings>
#include <QResizeEvent>
#include <QMenu>
#include <QLabel>
#include <QHBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
    static const QChar g_separator = '/';
    static const QChar g_windowsSeparator = '\\';
    static const QString g_labelName = QStringLiteral("BreadCrumbLabel");
    static const QString g_buttonName = QStringLiteral("MenuButton");
    // Separator is two non-breaking spaces, Right-Pointing Angle Quotation Mark (U+203A) and two more
    // non-breaking spaces
    static const QString g_plainTextSeparator = QStringLiteral("\u00a0\u00a0\u203a\u00a0\u00a0");
    static constexpr int g_iconWidth = 16;

    BreadCrumbs::BreadCrumbs(QWidget* parent)
        : QWidget(parent)
        , m_config(defaultConfig())
    {
        // WARNING: If you add any any new widget to the layout, make sure that it's accounted for in ::sizeHint() and ::fillLabel()
        
        // create the layout
        QHBoxLayout* boxLayout = new QHBoxLayout(this);
        boxLayout->setContentsMargins(0, 0, 0, 0);

        m_menuButton = new QToolButton(this);
        m_menuButton->setObjectName(g_buttonName);
        m_menuButton->setAutoRaise(true);
        boxLayout->addWidget(m_menuButton);
        connect(m_menuButton, &QToolButton::clicked, this, &BreadCrumbs::showTruncatedPathsMenu);

        // create the label
        m_label = new QLabel(this);
        m_label->setObjectName(g_labelName);
        m_label->setTextFormat(Qt::RichText);
        // We need to explicitly set indent. Otherwise the calculations are not correct because the
        // style causes the frame to be positive in size (but invisible) which gives us the effective indent
        // described in https://doc.qt.io/qt-5/qlabel.html#indent-prop
        m_label->setIndent(0);
        boxLayout->addWidget(m_label);
        // Horizontal policy deliberately ignored, we manage the width ourselves see ::sizeHint() and ::fillLabel()
        m_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        connect(m_label, &QLabel::linkActivated, this, &BreadCrumbs::onLinkActivated);
    }

    BreadCrumbs::~BreadCrumbs()
    {
    }

    bool BreadCrumbs::isBackAvailable() const
    {
        return !m_backPaths.isEmpty();
    }

    bool BreadCrumbs::isForwardAvailable() const
    {
        return !m_forwardPaths.isEmpty();
    }

    bool BreadCrumbs::isUpAvailable() const
    {
        int separatorIndex = m_currentPath.lastIndexOf(g_separator);
        return (separatorIndex > 0);
    }

    QString BreadCrumbs::currentPath() const
    {
        return m_currentPath;
    }

    void BreadCrumbs::setCurrentPath(const QString& newPath)
    {
        m_currentPath = newPath;

        // clean up the path to use all the first separator in the list of separators
        m_currentPath.replace(g_windowsSeparator, g_separator);

        // update internals
        m_currentPathSize = m_currentPath.split(g_separator, Qt::SkipEmptyParts).size();
        m_currentPathIcons.resize(m_currentPathSize);

        updateGeometry();
        fillLabel();
    }

    void BreadCrumbs::setDefaultIcon(const QString& icon)
    {
        m_defaultIcon = icon;
    }

    void BreadCrumbs::setIconAt(int index, const QString& icon)
    {
        if (index < 0 || index >= m_currentPathSize)
        {
            return;
        }

        m_currentPathIcons[index] = icon;
        updateGeometry();
        fillLabel();
    }
    
    QIcon BreadCrumbs::iconAt(int index)
    {
        if (index < 0 || index >= m_currentPathSize || m_currentPathIcons[index].isNull())
        {
            return QIcon(m_defaultIcon);
        }

        return QIcon(m_currentPathIcons[index]);
    }

    bool BreadCrumbs::getPushPathOnLinkActivation() const
    {
        return m_pushPathOnLinkActivation;
    }

    void BreadCrumbs::setPushPathOnLinkActivation(bool pushPath)
    {
        m_pushPathOnLinkActivation = pushPath;
    }

    QWidget* BreadCrumbs::createSeparator()
    {
        QFrame* line = new QFrame(this);
        line->setObjectName(QStringLiteral("breadcrumbs-separator"));
        line->setMinimumSize(QSize(0, 0));
        line->setMaximumSize(QSize(2, 16));
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Sunken);
        return line;
    }

    QSize BreadCrumbs::sizeHint() const
    {
        const QFontMetrics fm(m_label->font());
        const qreal separatorWidth = fm.horizontalAdvance(g_plainTextSeparator);
        // Icon adds the icon itself plus two spaces, see generateIconHtml()
        const qreal iconWidth = g_iconWidth + fm.horizontalAdvance("\u00a0\u00a0");

        // TODO(Qt 5.15.2): replace with:
        // const QList<QStringView> fullPath = QStringView(m_currentPath).split(g_separator, Qt::SkipEmptyParts);
        // to use views for avoiding allocations
        const QStringList fullPath = m_currentPath.split(g_separator, Qt::SkipEmptyParts);

        const int noSeparatorsWidth = AZStd::accumulate(fullPath.cbegin(), fullPath.cend(), 0,
            [&fm](int val, const QString& pathSegment ){
                return val + fm.horizontalAdvance(pathSegment);
            }
        );

        // separators are there only if path has more than one segment
        const qreal separatorsOnlyWidth = fullPath.size() > 1 ? (fullPath.size() - 1) * separatorWidth : .0;

        // assume that icon will be there if there's a path for a given icon or we have a default one
        const auto numIcons = !m_defaultIcon.isEmpty() ? fullPath.size()
            : AZStd::count_if( m_currentPathIcons.cbegin(), m_currentPathIcons.cend(), [](const QString& iconPath) {
                return !iconPath.isEmpty();
            }
        );

        QSize sh = QWidget::sizeHint();
        sh.rwidth() = qCeil(noSeparatorsWidth + separatorsOnlyWidth + numIcons * iconWidth);
        return sh;
    }

    QWidget* BreadCrumbs::createBackForwardToolBar()
    {
        QWidget* toolbar = new QWidget(this);
        QHBoxLayout* lay = new QHBoxLayout(toolbar);
        lay->setContentsMargins(10, 0, 10, 0);
        lay->setSpacing(10);
        lay->addWidget(createButton(NavigationButton::Back));
        lay->addWidget(createButton(NavigationButton::Forward));
        return toolbar;
    }

    QToolButton* BreadCrumbs::createButton(NavigationButton type)
    {
        QToolButton* button = new QToolButton(this);

        switch (type)
        {
        case NavigationButton::Back:
            button->setIcon(QIcon(":/Breadcrumb/img/UI20/Breadcrumb/arrow_left-default.svg"));
            button->setIconSize(QSize(16, 16));
            button->setEnabled(isBackAvailable());
            connect(button, &QToolButton::released, this, &BreadCrumbs::back);
            connect(this, &BreadCrumbs::backAvailabilityChanged, button, &QWidget::setEnabled);
            break;

        case NavigationButton::Forward:
            button->setIcon(QIcon(":/Breadcrumb/img/UI20/Breadcrumb/arrow_right-default.svg"));
            button->setEnabled(isForwardAvailable());
            button->setIconSize(QSize(16, 16));
            connect(button, &QToolButton::released, this, &BreadCrumbs::forward);
            connect(this, &BreadCrumbs::forwardAvailabilityChanged, button, &QWidget::setEnabled);
            break;

        case NavigationButton::Browse:
            button->setIcon(QIcon(":/Breadcrumb/img/UI20/Breadcrumb/List_View.svg"));
            button->setIconSize(QSize(16, 16));
            break;

        default:
            Q_UNREACHABLE();
            break;
        }

        return button;
    }

    void BreadCrumbs::pushPath(const QString& fullPath)
    {
        BreadCrumbButtonStates buttonStates;
        getButtonStates(buttonStates);

        if (!m_currentPath.isEmpty())
        {
            m_backPaths.push(m_currentPath);
        }

        m_forwardPaths.clear();

        changePath(fullPath);

        emitButtonSignals(buttonStates);
    }

    bool BreadCrumbs::back()
    {
        if (!isBackAvailable())
        {
            return false;
        }

        BreadCrumbButtonStates buttonStates;
        getButtonStates(buttonStates);

        m_forwardPaths.push(m_currentPath);

        QString newPath = m_backPaths.pop();

        changePath(newPath);

        emitButtonSignals(buttonStates);

        return true;
    }

    bool BreadCrumbs::forward()
    {
        if (!isForwardAvailable())
        {
            return false;
        }

        BreadCrumbButtonStates buttonStates;
        getButtonStates(buttonStates);

        m_backPaths.push(m_currentPath);

        QString newPath = m_forwardPaths.pop();

        changePath(newPath);

        emitButtonSignals(buttonStates);

        return true;
    }

    void BreadCrumbs::resizeEvent(QResizeEvent* event)
    {
        if (event->oldSize().width() != event->size().width())
        {
            const bool needsMenu = sizeHint().width() > event->size().width();
            m_menuButton->setVisible(needsMenu);
            fillLabel();
        }
        QWidget::resizeEvent(event);
    }

    QString BreadCrumbs::generateIconHtml(int index)
    {
        QString imagePath;

        if (index >= 0 && index < m_currentPathIcons.size())
        {
            imagePath = m_currentPathIcons[index];
        }

        if (imagePath.isEmpty())
        {
            imagePath = m_defaultIcon;
        }

        return !imagePath.isEmpty() ? QStringLiteral("<img width=\"%1\" height=\"%1\" style=\"vertical-align: middle\" src=\"%2\">%3%3")
                                          .arg(g_iconWidth)
                                          .arg(imagePath)
                                          .arg("&nbsp;")
                                    : QString{};
    }

    void BreadCrumbs::fillLabel()
    {
        QString htmlString = "";
        const QStringList fullPath = m_currentPath.split(g_separator, Qt::SkipEmptyParts);
        m_truncatedPaths = fullPath;

        // used to measure the width used by the path.
        const int availableWidth = width() -
            // using sizeHint() because width() will be the QWidget's default 100px before the first layouting
            (m_menuButton->isVisible() ? m_menuButton->sizeHint().width() + layout()->spacing() : 0);

        const QFontMetricsF fm(m_label->font());

        // Icon adds the icon itself plus two non-breaking spaces, see generateIconHtml()
        const qreal iconSpaceWidth = g_iconWidth + fm.horizontalAdvance(QStringLiteral("\u00a0\u00a0"));

        QString plainTextPath;

        auto formatLink = [this](const QString& fullPath, const QString& shortPath) -> QString {
            return QString("<a href=\"%1\" style=\"color: %2\">%3</a>").arg(fullPath, m_config.linkColor, shortPath);
        };

        const QString nonBreakingSpace = QStringLiteral("&nbsp;");
        // Using Single Right-Pointing Angle Quotation Mark (U+203A) as separator
        const QString arrowCharacter = QStringLiteral("&#8250;");
        auto prependSeparators = [&]() {
            htmlString.prepend(QStringLiteral("%1%1%2%1%1").arg(nonBreakingSpace, arrowCharacter));
        };

        if (m_truncatedPaths.isEmpty())
        {
            m_label->clear();
            return;
        }

        // last section is not clickable
        plainTextPath = m_truncatedPaths.takeLast();

        int index = m_currentPathSize - 1;

        // to estimate how much the rendered html will take, we need to take icons into account
        qreal totalIconsWidth = .0;

        const QString firstIconHtml = generateIconHtml(index);
        totalIconsWidth += firstIconHtml.isEmpty() ? .0 : iconSpaceWidth;
        htmlString.prepend(generateIconHtml(index) + plainTextPath);
        --index;

        if (!m_truncatedPaths.isEmpty())
        {
            prependSeparators();
            plainTextPath.prepend(g_plainTextSeparator);
        }

        while (!m_truncatedPaths.isEmpty())
        {
            const QString iconHtml = generateIconHtml(index--);
            totalIconsWidth += iconHtml.isEmpty() ? .0 : iconSpaceWidth;

            const QString plaintextWithNext = (m_truncatedPaths.size() == 1)
                ? (m_truncatedPaths.last() + plainTextPath) // if we're on the root path segment, don't put separator before it
                : (g_plainTextSeparator + m_truncatedPaths.last() + plainTextPath);

            if ((fm.horizontalAdvance(plaintextWithNext) + totalIconsWidth) > availableWidth)
            {
                break;
            }
            plainTextPath = plaintextWithNext;

            const QString linkPath = buildPathFromList(fullPath, m_truncatedPaths.size());
            const QString& part = m_truncatedPaths.takeLast();
            htmlString.prepend(QString("%1%2").arg(iconHtml, formatLink(linkPath, part)));

            if (!m_truncatedPaths.empty())
            {
                prependSeparators();
            }
        }

        m_label->setText(htmlString);
    }

    void BreadCrumbs::changePath(const QString& newPath)
    {
        setCurrentPath(newPath);
        updateGeometry();

        Q_EMIT pathChanged(m_currentPath);
    }

    void BreadCrumbs::getButtonStates(BreadCrumbButtonStates buttonStates)
    {
        buttonStates[EnumToConstExprInt(NavigationButton::Back)] = isBackAvailable();
        buttonStates[EnumToConstExprInt(NavigationButton::Forward)] = isForwardAvailable();
    }

    void BreadCrumbs::emitButtonSignals(BreadCrumbButtonStates previousButtonStates)
    {
        if (isBackAvailable() != previousButtonStates[EnumToConstExprInt(NavigationButton::Back)])
        {
            Q_EMIT backAvailabilityChanged(isBackAvailable());
        }

        if (isForwardAvailable() != previousButtonStates[EnumToConstExprInt(NavigationButton::Forward)])
        {
            Q_EMIT forwardAvailabilityChanged(isForwardAvailable());
        }
    }

    BreadCrumbs::Config BreadCrumbs::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<QString>(settings, QStringLiteral("LinkColor"), config.linkColor);

        return config;
    }

    BreadCrumbs::Config BreadCrumbs::defaultConfig()
    {
        Config config;

        config.linkColor = QStringLiteral("white");

        return config;
    }

    bool BreadCrumbs::polish(Style* style, QWidget* widget, const Config& config)
    {
        BreadCrumbs* breadCrumbs = qobject_cast<BreadCrumbs*>(widget);
        if (breadCrumbs != nullptr)
        {
            breadCrumbs->m_config = config;
            breadCrumbs->fillLabel();

            style->repolishOnSettingsChange(breadCrumbs);

            return true;
        }

        return false;
    }

    void BreadCrumbs::onLinkActivated(const QString& link)
    {
        int linkIndex = link.count(g_separator);
        Q_EMIT linkClicked(link, linkIndex);
        if (m_pushPathOnLinkActivation)
        {
            pushPath(link);
        }
    }

    QString BreadCrumbs::buildPathFromList(const QStringList& fullPath, int pos)
    {
        if (fullPath.size() < pos)
        {
            return QString();
        }

        QString path;
        for (int i = 0; i < pos; i++)
        {
            path.append(fullPath.value(i));
            if (i < pos - 1)
            {
                path.append(g_separator);
            }
        }

        return path;
    }

    void BreadCrumbs::showTruncatedPathsMenu()
    {
        QMenu hiddenPaths;
        for (int i = m_truncatedPaths.size() - 1; i >= 0; i--)
        {
            hiddenPaths.addAction(m_truncatedPaths.at(i), [this, i]() {
                onLinkActivated(buildPathFromList(m_truncatedPaths, i + 1));
            });
        }

        const auto position = m_menuButton->mapToGlobal(m_menuButton->geometry().bottomLeft());
        hiddenPaths.exec(position);
    }
} // namespace AzQtComponents

#include "Components/Widgets/moc_BreadCrumbs.cpp"
