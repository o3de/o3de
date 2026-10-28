/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Utilities/IconUtilities.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QToolButton>

namespace AzQtComponents
{
    const int g_errorBorderWidth = 1;

    static const QString g_customCheckStateClass = QStringLiteral("CustomCheckState");

    // (options below can be moved to the config file later if needed)
    // Size, in pixels, of the check / indeterminate mark drawn at the left of each dropdown item.
    static constexpr int g_checkMarkSize = 16;
    // Horizontal inset, in pixels, of that mark from the left edge of the item. The left padding
    // reserved for the mark is set in ComboBox.qss (QAbstractItemView::item) and must stay in sync.
    static constexpr int g_checkMarkLeftMargin = 4;
    // Minimum height, in pixels, of a dropdown item. Drives the popup row geometry from the delegate
    // so it stays in sync with the rendered item regardless of the combo box font size.
    static constexpr int g_minItemHeight = 24;

    //! The arrow of a flattened combo box (ComboBox.qss:64-75): 16 x 16, with its upper left corner at the point, painted as the sheet
    //! painted the `image` of a rule (QRenderRule::drawImage, qstylesheetstyle.cpp:1379-1384: QIcon::paint), which renders the SVG
    //! for the device ratio of the painter, not of the widget's screen
    static void drawFlattenedComboArrow(QPainter* painter, const QPoint& topLeft, bool enabled)
    {
        CachedIcon(enabled
            ? QStringLiteral(":/stylesheet/img/UI20/combobox-arrow.svg")
            : QStringLiteral(":/stylesheet/img/UI20/combobox-arrow-disabled.svg")).paint(painter, QRect(topLeft, QSize(16, 16)));
    }

    //! Everything private to this file that the flattened GUI adds. A named namespace, not an anonymous one: a unity build joins
    //! this file with others into one translation unit, where anonymous namespaces merge.
    namespace ComboBoxFlat
    {
        // ------------------------------------------------------------------------------
        // The look of a combo box: ComboBox.qss over the entry rule of BaseStyleSheet.qss:68-84. The rule is computed for each state
        // (QRenderRule, qstylesheetstyle.cpp:1860-2128) and the later of two rules of equal specificity wins a property: :hover
        // (:22-29), :focus and an open popup (:31-42), [HasError=true] (:44-51), :disabled (:53-57). The face of a state is therefore
        // the idle one with what its rule sets, and a margin, a border and a radius of its own where the rule changes them.
        // TOKENIZE: ComboBoxFieldColor, ComboBoxFocusFieldColor, ComboBoxDisabledFieldColor, ComboBoxTextColor, ComboBoxDisabledTextColor,
        //           ComboBoxHoverBorder, ComboBoxFocusBorder, ComboBoxErrorBorder
        // ------------------------------------------------------------------------------
        const QColor Field(0xCC, 0xCC, 0xCC);               // ComboBox.qss:19 (BaseStyleSheet.qss:76)
        const QColor FocusField(0xFF, 0xFF, 0xFF);          // ComboBox.qss:41
        const QColor DisabledField(0x66, 0x66, 0x66);       // ComboBox.qss:55
        const QColor Text(0x00, 0x00, 0x00);                // BaseStyleSheet.qss:77
        const QColor DisabledText(0x99, 0x99, 0x99);        // ComboBox.qss:56
        const QColor HoverBorder(0, 0, 0, 50);              // ComboBox.qss:28
        const QColor FocusBorder(0x94, 0xD2, 0xFF);         // ComboBox.qss:39
        const QColor ErrorBorder(0xE2, 0x52, 0x43);         // ComboBox.qss:50

        constexpr QMargins NoMargin(0, 0, 0, 0);

        //! QComboBox, and :hover, :focus, an open popup and :disabled of it. The box: margin 1px (ComboBox.qss:17), padding-left 4px (:16),
        //! contents 16px high (:15), radius 2px and no border (BaseStyleSheet.qss:79-80: a radius is a border of the rule's own).
        //! :hover and :focus take the margin away for a 1px border and a radius of 3px (:24-28, :35-39); a popup that is open is
        //! `QComboBox[HasPopupOpen=true]` (:32-33), a property the combo box keeps while it shows its popup, which the style sees as the
        //! state QStyle::State_On (qcombobox.cpp:1499-1500). The captures hold the idle, :focus (the Gallery's page selector has the focus in
        //! n2/03_button), :disabled and error faces. UNVERIFIED against a capture: :hover (no combo box is under the pointer in any), an
        //! open popup (none is shown in a capture of a page) and the error box in a state of its own.
        const BoxElement Combo{
            .m_name = "Global.Box.ComboBox",
            .m_margin = QMargins(1, 1, 1, 1),               // ComboBox.qss:17
            .m_padding = QMargins(4, 0, 0, 0),              // ComboBox.qss:16
            .m_nativeBorder = false,                        // BaseStyleSheet.qss:79-80
            .m_radius = 2,                                  // BaseStyleSheet.qss:79
            .m_size = QSize(-1, 16),                        // ComboBox.qss:15
            .m_idle = { .m_fill = QBrush(Field), .m_text = Text },
            .m_stateFaces = {
                { BoxElement::Hover, {
                    .m_fill = QBrush(Field), .m_edges = BoxElement::border(1, HoverBorder), .m_text = Text,
                    .m_margin = NoMargin, .m_radius = 3 } },                                                          // :22-29
                { BoxElement::Focus, {
                    .m_fill = QBrush(FocusField), .m_edges = BoxElement::border(1, FocusBorder), .m_text = Text,
                    .m_margin = NoMargin, .m_radius = 3 } },                                                          // :31-42
                { BoxElement::Checked, {
                    .m_fill = QBrush(FocusField), .m_edges = BoxElement::border(1, FocusBorder), .m_text = Text,
                    .m_margin = NoMargin, .m_radius = 3 } },                                                          // :32-42
                { BoxElement::Disabled, { .m_fill = QBrush(DisabledField), .m_text = DisabledText } },                // :53-57
            },
        };

        //! QComboBox[HasError=true]: margin 0, a 2px border of #E25243, radius 4px (ComboBox.qss:44-51), over the box above. The rules of
        //! the other states set no margin, border or radius that outranks it (:22-42 come before it), so the border holds in them, with
        //! the fill of :focus and :disabled; an open popup (`[HasPopupOpen=true][HasPopupOpen=true]`, :33, one attribute more) outranks it.
        const BoxElement ComboError{
            .m_name = "Global.Box.ComboBox.Error",
            .m_margin = NoMargin,                           // ComboBox.qss:46
            .m_padding = QMargins(4, 0, 0, 0),              // ComboBox.qss:16
            .m_hasBox = true,                               // a margin of 0 is a margin
            .m_nativeBorder = false,                        // ComboBox.qss:47-48
            .m_radius = 4,                                  // ComboBox.qss:49
            .m_size = QSize(-1, 16),                        // ComboBox.qss:15
            .m_idle = { .m_fill = QBrush(Field), .m_edges = BoxElement::border(2, ErrorBorder), .m_text = Text },   // :47-50
            .m_stateFaces = {
                { BoxElement::Focus, { .m_fill = QBrush(FocusField), .m_edges = BoxElement::border(2, ErrorBorder), .m_text = Text } },
                { BoxElement::Checked, {
                    .m_fill = QBrush(FocusField), .m_edges = BoxElement::border(1, FocusBorder), .m_text = Text, .m_radius = 3 } },
                { BoxElement::Disabled, { .m_fill = QBrush(DisabledField), .m_edges = BoxElement::border(2, ErrorBorder), .m_text = DisabledText } },
            },
        };

        //! The look of a combo box that has no element of a module: the error one while it is in error
        const BoxElement* lookOf(const QWidget* comboBox)
        {
            return (comboBox && comboBox->property(HasError).toBool()) ? &ComboError : &Combo;
        }

        //! The element of a combo box, where the polish has not resolved one yet
        const BoxElement* elementOf(const QWidget* widget)
        {
            const BoxElement* resolved = BoxElement::forWidget(widget);
            return resolved ? resolved : (qobject_cast<const QComboBox*>(widget) ? lookOf(widget) : nullptr);
        }
    } // namespace ComboBoxFlat

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
                        // ignore properties coming from inside the style sheet system itself, which are all by convention
                        // prefixed with _q_, and those the flattened elements keep on a widget while the style polishes it (_o3de_)
                        QDynamicPropertyChangeEvent* eventFull = static_cast<QDynamicPropertyChangeEvent*>(event);
                        QString propertyName = QString::fromUtf8(eventFull->propertyName());
                        if (!propertyName.startsWith(QStringLiteral("_q_")) && !propertyName.startsWith(QStringLiteral("_o3de_")))
                        {
                            if (StyleManager::stylesheetsDisabled())
                            {
                                // The flattened GUI has no style sheet style to polish the combo box again, and what a property changes
                                // in it is its element (a combo box in error is another element): resolved again, as the polish ends
                                // with it. A whole polish would also connect the signals and replace the item delegate again, each
                                // time the popup opens or closes (the property HasPopupOpen).
                                BoxElement::apply(cbWidget);
                            }
                            // None before the StyleManager exists, as in the unit tests (ScrollBar.cpp and TreeView.cpp guard the same call)
                            else if (auto styleSheet = StyleManager::styleSheetStyle(cbWidget))
                            {
                                styleSheet->unpolish(cbWidget);
                                styleSheet->polish(cbWidget);
                            }
                        }
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

            // Prevent QT from drawing the checkbox as we do it ourself
            option->features &= ~QStyleOptionViewItem::HasCheckIndicator;
        }

        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QSize size = QStyledItemDelegate::sizeHint(option, index);
            size.setHeight(qMax(size.height(), g_minItemHeight));
            return size;
        }

        Qt::CheckState itemCheckState(const QModelIndex& index) const
        {
            if (Style::hasClass(m_combo, g_customCheckStateClass))
            {
                return index.data(Qt::CheckStateRole).value<Qt::CheckState>();
            }
            return (m_combo && m_combo->currentIndex() == index.row()) ? Qt::Checked : Qt::Unchecked;
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QStyledItemDelegate::paint(painter, option, index);

            if (!m_combo || m_combo->view()->selectionMode() != QAbstractItemView::SingleSelection)
            {
                return;
            }

            const Qt::CheckState checkState = itemCheckState(index);
            if (checkState == Qt::Unchecked)
            {
                return;
            }

            static const QIcon s_checkMark(QStringLiteral(":/stylesheet/img/UI20/checkmark.svg"));
            static const QIcon s_indeterminateMark(QStringLiteral(":/stylesheet/img/UI20/indeterminate.svg"));
            const QIcon& mark = (checkState == Qt::PartiallyChecked) ? s_indeterminateMark : s_checkMark;

            const QRect markRect(
                option.rect.left() + g_checkMarkLeftMargin,
                option.rect.top() + (option.rect.height() - g_checkMarkSize) / 2,
                g_checkMarkSize,
                g_checkMarkSize);
            mark.paint(painter, markRect, Qt::AlignCenter);
        }

    private:
        QPointer<QComboBox> m_combo;
    };

    void ComboBox::registerFlatElements()
    {
        static bool registered = false;
        if (registered || !StyleManager::stylesheetsDisabled())
        {
            return;
        }
        registered = true;

        // The look of every combo box (a module's own element is checked before these), the error one first: the rules of
        // [HasError=true] outrank the others (ComboBoxFlat). The element is resolved at polish, and a property that changes resolves it
        // again (ComboBoxWatcher).
        constexpr BoxElement::Scope global = BoxElement::Scope::Global;
        BoxElement::registerElement(&ComboBoxFlat::ComboError, &QComboBox::staticMetaObject,
            [](const QWidget* w) { return w->property(HasError).toBool(); }, global);
        BoxElement::registerElement(&ComboBoxFlat::Combo, &QComboBox::staticMetaObject, [](const QWidget*) { return true; }, global);
    }

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

        // The element sizes its combo box as its rule did (qstylesheetstyle.cpp:5336-5347): the box around the contents within the
        // declared size, the drop-down and the 2px QCommonStyle keeps in the label, where the rule has a box or a border of its own, else
        // the base style does. The element is a module's, else the look of the sheet's own rules (ComboBoxFlat), the Native one: not the
        // base style's size, which grows with QComboBox::frame (a frameless combo box was 2px too narrow).
        if (StyleManager::stylesheetsDisabled())
        {
            if (const BoxElement* box = ComboBoxFlat::elementOf(widget))
            {
                const QSize contents = box->adjustSize(size);
                return box->ownsBox(option) ? box->comboBoxSize(option, widget, contents)
                                            : style->QProxyStyle::sizeFromContents(type, option, contents, widget);
            }
        }

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

        // The element frames its combo box as its rule did (qstylesheetstyle.cpp:3067-3086): the rule over the whole rect, then the arrow in
        // its drop-down. The element is a module's, else the look of the sheet's own rules (ComboBoxFlat). UNVERIFIED: a fill under the
        // style's own border (:3053-3066) stays with the base style below; no element has one. Frameless combo boxes (setFrame(false))
        // render the same box.
        if (StyleManager::stylesheetsDisabled() && opt)
        {
            if (const BoxElement* box = ComboBoxFlat::elementOf(widget); box && !box->hasNativeBorder(option))
            {
                box->draw(painter, option, widget);
                if (opt->subControls & QStyle::SC_ComboBoxArrow)
                {
                    drawFlattenedComboArrow(painter, box->arrowRect(option, widget).topLeft(), opt->state.testFlag(QStyle::State_Enabled));
                }
                return true;
            }
        }

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

        // The element lays the label out in its edit field as its rule did (qstylesheetstyle.cpp:4021-4056)
        if (StyleManager::stylesheetsDisabled() && opt)
        {
            if (const BoxElement* box = ComboBoxFlat::elementOf(widget); box && box->drawComboBoxLabel(style, opt, painter, widget))
            {
                return true;
            }
        }

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

    void ComboBox::addErrorButton(QComboBox* cb, const Config& config)
    {
        auto errorToolButton = cb->findChild<QToolButton*>(ErrorToolButton);
        if (!errorToolButton)
        {
            auto toolButton = new QToolButton(cb);
            toolButton->setObjectName(ErrorToolButton);
            toolButton->setIcon(CachedIcon(config.errorImage, config.errorImageSize));
            toolButton->resize(config.errorImageSize.width(), config.errorImageSize.height());
            toolButton->hide();
        }
    }

    void ComboBox::addCustomCheckStateStyle(QComboBox* cb)
    {
        Style::addClass(cb, g_customCheckStateClass);
    }

} // namespace AzQtComponents

