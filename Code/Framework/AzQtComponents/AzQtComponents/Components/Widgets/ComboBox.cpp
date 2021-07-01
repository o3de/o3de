/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDesktopWidget>
#include <QGraphicsDropShadowEffect>
#include <QLineEdit>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QToolButton>

#include <QtWidgets/private/qstylesheetstyle_p.h>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
    const int g_errorBorderWidth = 1;

    static const QString g_customCheckStateClass = QStringLiteral("CustomCheckState");

    ComboBoxValidator::ComboBoxValidator(QObject* parent)
        : QValidator(parent)
    {
    }

    QValidator::State ComboBoxValidator::validateIndex(int) const
    {
        return QValidator::Acceptable;
    }

    QValidator::State ComboBoxValidator::validate(QString&, int&) const
    {
        return QValidator::Acceptable;
    }

    class ComboBoxWatcher : public QObject
    {
    public:
        explicit ComboBoxWatcher(QObject* parent = nullptr)
            : QObject(parent)
        {
        }

        bool isError(QComboBox* cb) const
        {
            auto validator = cb->property(Validator);

            if (validator.canConvert<AzQtComponents::ComboBoxValidator*>())
            {
                auto validatorObj = validator.value<AzQtComponents::ComboBoxValidator*>();
                return validatorObj->validateIndex(cb->currentIndex()) != QValidator::Acceptable;
            }
            else if (validator.canConvert<QValidator*>())
            {
                auto validatorObj = validator.value<QValidator*>();
                int cursor = 0;
                QString textCopy = cb->currentText();
                return validatorObj->validate(textCopy, cursor) != QValidator::Acceptable;
            }

            return false;
        }

        void updateErrorState(QComboBox* cb)
        {
            const bool error = isError(cb);

            ComboBox::setError(cb, error);

            auto errorToolButton = cb->findChild<QToolButton*>(ErrorToolButton);

            if (errorToolButton)
            {
                errorToolButton->setVisible(error);
            }

            positionSideWidgets(cb);
        }

        void positionSideWidgets(QComboBox* cb)
        {
            auto errorToolButton = cb->findChild<QToolButton*>(ErrorToolButton);

            if (errorToolButton)
            {
                auto rect = cb->rect();
                QStyleOptionComboBox opt;
                opt.subControls &= QStyle::SC_ComboBoxArrow;
                auto arrowButtonRect = cb->style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, cb);
                QRect widgetGeometry(QPoint(
                    rect.width() - g_errorBorderWidth - errorToolButton->width() - arrowButtonRect.width() - m_config.errorImageSpacing,
                    (rect.height() - errorToolButton->height()) / 2),
                    errorToolButton->size());
                errorToolButton->setGeometry(widgetGeometry);
            }
        }

        void updateErrorStateSlot()
        {
            if (auto cb = qobject_cast<QComboBox*>(sender()))
            {
                updateErrorState(cb);
            }
        }

        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (auto cbWidget = qobject_cast<QComboBox*>(watched))
            {
                switch (event->type())
                {
                    case QEvent::Show:
                    {
                        // Need to check validity also for the preselected item
                        updateErrorState(cbWidget);
                    }
                    break;

                    case QEvent::Resize:
                    {
                        positionSideWidgets(cbWidget);
                    }
                    break;

                    case QEvent::DynamicPropertyChange:
                    {
                        auto styleSheet = StyleManager::styleSheetStyle(cbWidget);
                        styleSheet->repolish(cbWidget);
                    }
                    break;

                    // Only allow wheel events when the combo box has focus
                    case QEvent::Wheel:
                    {
                        if (cbWidget->hasFocus())
                        {
                            event->accept();
                            return false;
                        }
                        else
                        {
                            event->ignore();
                            return true;
                        }
                    }
                    break;
                }
            }
            else if (auto itemView = qobject_cast<QAbstractItemView*>(watched))
            {
                bool newHasPopupOpen = false;

                switch (event->type())
                {
                    case QEvent::Show:
                    {
                        newHasPopupOpen = true;
                    }
                    // intentional fall-through
                    case QEvent::Hide:
                    {
                        if (!itemView->parent())
                        {
                            return false;
                        }

                        auto cb = qobject_cast<QComboBox*>(itemView->parent()->parent());

                        if (!cb)
                        {
                            return false;
                        }

                        if (cb->property(HasPopupOpen).toBool() != newHasPopupOpen)
                        {
                            cb->setProperty(HasPopupOpen, newHasPopupOpen);
                        }
                    }
                    break;
                }
            }

            return QObject::eventFilter(watched, event);
        }

    private:
        friend class ComboBox;
        ComboBox::Config m_config;
    };

    class ComboBoxItemDelegate : public QStyledItemDelegate
    {
    public:
        explicit ComboBoxItemDelegate(QComboBox* combo, QWidget* parent = nullptr)
            : QStyledItemDelegate(parent)
            , m_combo(combo)
        {
        }

    protected:
        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
        {
            QStyledItemDelegate::initStyleOption(option, index);

            // The default usage of combobox only allow one selection,
            // then we have to tweak it a bit so it show a check mark in this case.
            if (option && m_combo && m_combo->view()->selectionMode() == QAbstractItemView::SingleSelection)
            {
                option->features |= QStyleOptionViewItem::HasCheckIndicator;
                auto *style = qobject_cast<Style *>(m_combo->style());
                Qt::CheckState checkState;
                if (style && style->hasClass(m_combo, g_customCheckStateClass))
                {
                    checkState = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
                }
                else
                {
                    checkState = m_combo->currentIndex() == index.row() ? Qt::Checked : Qt::Unchecked;
                }
                option->checkState = checkState;
            }
        }

    private:
        QPointer<QComboBox> m_combo;
    };

    ComboBox::Config ComboBox::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<QColor>(settings, QStringLiteral("PlaceHolderTextColor"), config.placeHolderTextColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("FramelessTextColor"), config.framelessTextColor);
        ConfigHelpers::read<QString>(settings, QStringLiteral("ErrorImage"), config.errorImage);
        ConfigHelpers::read<QSize>(settings, QStringLiteral("ErrorImageSize"), config.errorImageSize);
        ConfigHelpers::read<int>(settings, QStringLiteral("ErrorImageSpacing"), config.errorImageSpacing);
        ConfigHelpers::read<int>(settings, QStringLiteral("ItemPadding"), config.itemPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("LeftAdjust"), config.leftAdjust);

        return config;
    }

    ComboBox::Config ComboBox::defaultConfig()
    {
        Config config;

        config.placeHolderTextColor = QColor("#999999");
        config.framelessTextColor = QColor(Qt::white);
        config.errorImage = QStringLiteral(":/stylesheet/img/UI20/lineedit-error.svg");
        config.errorImageSize = {16, 16};
        config.errorImageSpacing = 4;
        config.itemPadding = 0; // Adjust the size to line up the right side of the combobox.
        config.leftAdjust = 26; // Line up the dropdown items with the box contents.
        return config;
    }

    void ComboBox::setValidator(QComboBox* cb, QValidator* validator)
    {
        cb->setProperty(Validator, QVariant::fromValue(validator));
    }

    void ComboBox::setError(QComboBox* cb, bool error)
    {
        if (error != cb->property(HasError).toBool())
        {
            cb->setProperty(HasError, error);
        }
    }

    QPointer<ComboBoxWatcher> ComboBox::s_comboBoxWatcher = nullptr;
    unsigned int ComboBox::s_watcherReferenceCount = 0;

    void ComboBox::initializeWatcher()
    {
        if (!s_comboBoxWatcher)
        {
            Q_ASSERT(s_watcherReferenceCount == 0);
            s_comboBoxWatcher = new ComboBoxWatcher;
        }

        ++s_watcherReferenceCount;
    }

    void ComboBox::uninitializeWatcher()
    {
        Q_ASSERT(!s_comboBoxWatcher.isNull());
        Q_ASSERT(s_watcherReferenceCount > 0);

        --s_watcherReferenceCount;

        if (s_watcherReferenceCount == 0)
        {
            delete s_comboBoxWatcher;
            s_comboBoxWatcher = nullptr;
        }
    }

    bool ComboBox::polish(Style* style, QWidget* widget, const ComboBox::Config& config)
    {
        auto comboBox = qobject_cast<QComboBox*>(widget);

        if (comboBox)
        {
            s_comboBoxWatcher->m_config = config;
            QObject::connect(comboBox, qOverload<int>(&QComboBox::currentIndexChanged), s_comboBoxWatcher, &ComboBoxWatcher::updateErrorStateSlot);

            // The default menu combobox delegate is not stylishable via qss
            comboBox->setItemDelegate(new ComboBoxItemDelegate(comboBox, comboBox->view()));

            if (QLineEdit* le = comboBox->lineEdit())
            {
                QPalette pal = le->palette();
#if !defined(AZ_PLATFORM_LINUX)
                pal.setColor(QPalette::PlaceholderText, config.placeHolderTextColor);
#endif //!defined(AZ_PLATFORM_LINUX)
                le->setPalette(pal);
            }

            style->repolishOnSettingsChange(comboBox);

            comboBox->installEventFilter(s_comboBoxWatcher);
            if (comboBox->view())
            {
                comboBox->view()->installEventFilter(s_comboBoxWatcher);
            }

            // Prevent QComboBoxes from automatically gaining focus on a wheel event
            // so that scrolling a parent does not result in unintended combo box changes
            if (comboBox->focusPolicy() == Qt::FocusPolicy::WheelFocus)
            {
                comboBox->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
            }

            ComboBox::addErrorButton(comboBox, config);
        }

        return comboBox;
    }

    bool ComboBox::unpolish(Style* style, QWidget* widget, const ComboBox::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        auto comboBox = qobject_cast<QComboBox*>(widget);

        if (comboBox)
        {
            QWidget* frame = comboBox->view()->parentWidget();
            frame->setWindowFlags(frame->windowFlags() & ~(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint));
            frame->setAttribute(Qt::WA_TranslucentBackground, false);

            frame->setContentsMargins(QMargins());
            delete frame->graphicsEffect();
        }

        return comboBox;
    }

    QSize ComboBox::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const ComboBox::Config& config)
    {
        Q_UNUSED(config);

        QSize comboBoxSize = style->QProxyStyle::sizeFromContents(type, option, size, widget);

        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (opt && opt->frame)
        {
            return comboBoxSize;
        }

        const int indicatorWidth = style->proxy()->pixelMetric(QStyle::PM_IndicatorWidth, option, widget);
        const int margin = style->proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin, option, widget) + 2;
        comboBoxSize.setWidth(comboBoxSize.width() + indicatorWidth + 3 * margin);
        return comboBoxSize;
    }

    QRect ComboBox::comboBoxListBoxPopupRect(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        QRect r = style->QCommonStyle::subControlRect(QStyle::CC_ComboBox, static_cast<const QStyleOptionComplex*>(option), QStyle::SC_ComboBoxListBoxPopup, widget);
        r.setWidth(r.width() + config.itemPadding);
        r.setLeft(r.left() - config.leftAdjust);
        return r;
    }

    bool ComboBox::drawComboBox(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(config);

        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (!opt || opt->frame)
        {
            return false;
        }

        // flat combo box
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, option, painter, widget);
        return true;
    }

    bool ComboBox::drawComboBoxLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (!opt || opt->frame)
        {
            return false;
        }

        QStyleOptionComboBox cb(*opt);
        cb.palette.setColor(QPalette::Text, config.framelessTextColor);
        const QRect editRect = style->proxy()->subControlRect(QStyle::CC_ComboBox, &cb, QStyle::SC_ComboBoxEditField, widget);
        style->proxy()->drawItemText(painter, editRect.adjusted(1, 0, -1, 0), Qt::AlignLeft | Qt::AlignVCenter, cb.palette, cb.state & QStyle::State_Enabled, cb.currentText, QPalette::Text);
        return true;
    }

    bool ComboBox::drawIndicatorArrow(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (!opt || opt->frame)
        {
            return false;
        }

        QStyleOptionComboBox cb(*opt);
        const QRect downArrowRect = style->proxy()->subControlRect(QStyle::CC_ComboBox, &cb, QStyle::SC_ComboBoxArrow, widget);
        cb.rect = downArrowRect;
        cb.palette.setColor(QPalette::ButtonText, config.framelessTextColor);
        style->QCommonStyle::drawPrimitive(QStyle::PE_IndicatorArrowDown, &cb, painter, widget);
        return true;
    }

    bool ComboBox::drawItemCheckIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(config);

        if (strcmp(widget->metaObject()->className(), "QComboBoxListView") != 0)
        {
            return false;
        }

        auto opt = qstyleoption_cast<const QStyleOptionViewItem*>(option);
        if (!opt)
        {
            return false;
        }

        if (opt->checkState != Qt::Unchecked)
        {
            style->QProxyStyle::drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, opt, painter, widget);
        }

        return true;
    }

    void ComboBox::addErrorButton(QComboBox* cb, const Config& config)
    {
        auto errorToolButton = cb->findChild<QToolButton*>(ErrorToolButton);
        if (!errorToolButton)
        {
            QIcon errorIcon;
            errorIcon.addFile(config.errorImage, config.errorImageSize);
            auto toolButton = new QToolButton(cb);
            toolButton->setObjectName(ErrorToolButton);
            toolButton->setIcon(errorIcon);
            toolButton->resize(config.errorImageSize.width(), config.errorImageSize.height());
            toolButton->hide();
        }
    }

    void ComboBox::addCustomCheckStateStyle(QComboBox* cb)
    {
        Style::addClass(cb, g_customCheckStateClass);
    }

} // namespace AzQtComponents

#include <Components/Widgets/moc_ComboBox.cpp>
