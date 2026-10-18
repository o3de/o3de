/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/LineEditRevertHandler.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <QPushButton>
#include <QHBoxLayout>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QTextFormat::d': class 'QSharedDataPointer<QTextFormatPrivate>' needs to have dll-interface to be used by clients of class 'QTextFormat'
#include <QLineEdit>
AZ_POP_DISABLE_WARNING
#include <QIcon>
#include <QPainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QSettings>
#include <QPushButton>
#include <QToolTip>
#include <QAction>
#include <QPainterPath>
#include <QStyleOptionFrame>

namespace AzQtComponents
{
    static const QString ClearButtonActionNameC = QStringLiteral("_q_qlineeditclearaction");

    //! Everything private to this file that the flattened GUI adds. A named namespace, not an anonymous one: a unity build joins
    //! this file with others into one translation unit, where anonymous namespaces merge.
    namespace BrowseEditFlat
    {
        // The box of a BrowseEdit and of the line edit inside it, as the legacy rules gave them. The margin is the BrowseEdit's own
        // (BrowseEdit.qss:10-13); the rest is the rule `AzQtComponents--BrowseEdit QLineEdit` (BrowseEdit.qss:15-29), which outranks
        // the QLineEdit rules of LineEdit.qss:11-20 and BaseStyleSheet.qss:68-84 wherever it sets a property: no border, the two left
        // corners rounded (the right ones square: the attached button's), padding 2 on the left and -4 on the right, and the vertical
        // padding LineEdit.qss gave every line edit (-1 above, 1 below) that it leaves alone.
        // TOKENIZE: BrowseEditMargin, BrowseEditFieldRadius, BrowseEditFieldPadding
        constexpr int Margin = 2;                       // BrowseEdit.qss:12
        constexpr int FieldRadius = 2;                  // BrowseEdit.qss:21-22 (:23-24 the right corners, 0)
        constexpr int PaddingLeft = 2;                  // BrowseEdit.qss:26
        constexpr int PaddingRight = -4;                // BrowseEdit.qss:28
        constexpr int PaddingTop = -1;                  // LineEdit.qss:19
        constexpr int PaddingBottom = 1;                // LineEdit.qss:18

        //! QRenderRule::borderClip (qstylesheetstyle.cpp:1288-1325) of that rule: a path along the box with the arcs of its two left
        //! corners (no border, so none of the half-widths the arcs lose); QRenderRule::drawBackground fills it antialiased
        //! (:1348-1370), so at a fractional device ratio the rows at the edges blend.
        QPainterPath fieldPath(const QRectF& box)
        {
            const qreal radius = FieldRadius;
            QPainterPath path;
            path.moveTo(box.left() + radius, box.top());
            path.lineTo(box.right(), box.top());
            path.lineTo(box.right(), box.bottom());
            path.lineTo(box.left() + radius, box.bottom());
            path.arcTo(QRectF(box.left(), box.bottom() - 2 * radius, 2 * radius, 2 * radius), 270, -90);
            path.lineTo(box.left(), box.top() + radius);
            path.arcTo(QRectF(box.left(), box.top(), 2 * radius, 2 * radius), 180, -90);
            path.closeSubpath();
            return path;
        }
    } // namespace BrowseEditFlat

    struct BrowseEdit::InternalData
    {
        QLineEdit* m_lineEdit = nullptr;
        QPushButton* m_attachedButton = nullptr;
        QString m_errorToolTip;
        bool m_hasAcceptableInput = true;
    };

    BrowseEdit::BrowseEdit(QWidget* parent /* = nullptr */)
        : QFrame(parent)
        , m_data(new BrowseEdit::InternalData)
    {
        setObjectName("browse-edit");
        setToolTipDuration(1000 * 10);
        setAttribute(Qt::WA_Hover, true);

        QHBoxLayout* boxLayout = new QHBoxLayout(this);

        boxLayout->setContentsMargins(0, 0, 0, 0);
        boxLayout->setSpacing(0);

        m_data->m_lineEdit = new QLineEdit(this);
        // Do not draw custom style, but we polish/unpolish manually to use
        // custom LineEdit behaviour
        Style::flagToIgnore(m_data->m_lineEdit);
        setFocusProxy(m_data->m_lineEdit);
        m_data->m_lineEdit->setObjectName("line-edit");
        m_data->m_lineEdit->installEventFilter(this);
        new LineEditRevertHandler(m_data->m_lineEdit, this);
        LineEdit::setEnableClearButtonWhenReadOnly(m_data->m_lineEdit, true);
        boxLayout->addWidget(m_data->m_lineEdit);

        connect(m_data->m_lineEdit, &QLineEdit::returnPressed, this, &BrowseEdit::returnPressed);
        connect(m_data->m_lineEdit, &QLineEdit::editingFinished, this, &BrowseEdit::editingFinished);
        connect(m_data->m_lineEdit, &QLineEdit::textChanged, this, &BrowseEdit::onTextChanged);
        connect(m_data->m_lineEdit, &QLineEdit::textEdited, this, &BrowseEdit::textEdited);

        m_data->m_attachedButton = new QPushButton(this);
        m_data->m_attachedButton->setObjectName("attached-button");
        if (StyleManager::stylesheetsDisabled())
        {
            // BrowseEdit.qss: #attached-button flat, 16x16, icon 12x12; the inner QLineEdit has
            // border-width 0 (its field colour comes from the palette, the frame from
            // BrowseEdit::drawFrame). TOKENIZE: BrowseEditButtonSize, BrowseEditButtonIconSize
            m_data->m_attachedButton->setFlat(true);
            m_data->m_attachedButton->setFixedSize(16, 16);
            m_data->m_attachedButton->setIconSize(QSize(12, 12));
            m_data->m_lineEdit->setFrame(false);

            // The padding of the rule (BrowseEdit.qss:26, :28; LineEdit.qss:18-19) is where the sheet put the text: inside the field's
            // rect, 2px from its left edge (and the rect that holds it 4px wider on the right, a line less above and a line more
            // below), which also sets the size hint of the field. A line edit keeps that room as text margins.
            m_data->m_lineEdit->setTextMargins(BrowseEditFlat::PaddingLeft, BrowseEditFlat::PaddingTop,
                BrowseEditFlat::PaddingRight, BrowseEditFlat::PaddingBottom);
        }
        boxLayout->addWidget(m_data->m_attachedButton);
        connect(m_data->m_attachedButton, &QPushButton::clicked, this, &BrowseEdit::attachedButtonTriggered);
        setAttachedButtonIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));

        AzQtComponents::Style::addClass(this, "AzQtComponentsBrowseEdit");
    }

    BrowseEdit::~BrowseEdit()
    {
    }

    void BrowseEdit::setAttachedButtonIcon(const QIcon& icon)
    {
        m_data->m_attachedButton->setIcon(icon);

        m_data->m_attachedButton->setVisible(!icon.isNull());
    }

    QIcon BrowseEdit::attachedButtonIcon() const
    {
        return m_data->m_attachedButton->icon();
    }

    // use a separate button, because UX spec calls for a clear button to be enabled when the line edit
    // is not
    // connect this to the text changed signal
    bool BrowseEdit::isClearButtonEnabled() const
    {
        return m_data->m_lineEdit->isClearButtonEnabled();
    }

    void BrowseEdit::setClearButtonEnabled(bool enable)
    {
        if (isClearButtonEnabled() != enable)
        {
            m_data->m_lineEdit->setClearButtonEnabled(enable);
        }
    }

    bool BrowseEdit::isLineEditReadOnly() const
    {
        return m_data->m_lineEdit->isReadOnly();
    }

    void BrowseEdit::setLineEditReadOnly(bool readOnly)
    {
        m_data->m_lineEdit->setReadOnly(readOnly);
    }

    void BrowseEdit::setPlaceholderText(const QString& placeholderText)
    {
        m_data->m_lineEdit->setPlaceholderText(placeholderText);
    }

    QString BrowseEdit::placeholderText() const
    {
        return m_data->m_lineEdit->placeholderText();
    }

    void BrowseEdit::setText(const QString& text)
    {
        m_data->m_lineEdit->setText(text);
    }

    QString BrowseEdit::text() const
    {
        return m_data->m_lineEdit->text();
    }

    void BrowseEdit::setErrorToolTip(const QString& errorToolTip)
    {
        if (errorToolTip != m_data->m_errorToolTip)
        {
            m_data->m_errorToolTip = errorToolTip;
            LineEdit::setErrorMessage(m_data->m_lineEdit, errorToolTip);
        }
    }

    QString BrowseEdit::errorToolTip() const
    {
        return m_data->m_errorToolTip;
    }

    void BrowseEdit::setValidator(const QValidator* validator)
    {
        m_data->m_lineEdit->setValidator(validator);
        m_data->m_hasAcceptableInput = m_data->m_lineEdit->hasAcceptableInput();
    }

    const QValidator* BrowseEdit::validator() const
    {
        return m_data->m_lineEdit->validator();
    }

    bool BrowseEdit::hasAcceptableInput() const
    {
        return m_data->m_lineEdit->hasAcceptableInput();
    }

    QLineEdit* BrowseEdit::lineEdit() const
    {
        return m_data->m_lineEdit;
    }

    BrowseEdit::Config BrowseEdit::loadConfig(QSettings& settings)
    {
        return LineEdit::loadConfig(settings);
    }

    BrowseEdit::Config BrowseEdit::defaultConfig()
    {
        return LineEdit::defaultConfig();
    }

    // The field of the line edit inside a BrowseEdit, as the sheets painted it: the QLineEdit
    // rule gave #CCCCCC with black text (BaseStyleSheet.qss:76-77), :disabled #666666 with #999999
    // text (LineEdit.qss:33-34). .ValidDropTarget QLineEdit { background-color: transparent }
    // (BrowseEdit.qss:35: the blue frame fill painted by BrowseEdit::drawFrame shows through) and
    // .InvalidDropTarget QLineEdit { background-color: #FFFFFF } (BrowseEdit.qss:41) outrank
    // :disabled, so they hold in every state. A style sheet derives the placeholder from the text
    // color at half alpha (qstylesheetstyle.cpp:1459-1463): #80000000, #80999999 when disabled.
    // Runs after LineEdit::polish, whose #888888 placeholder it replaces. Style::addClass and
    // removeClass polish the BrowseEdit, so a drop-target class change lands here too.
    // TOKENIZE: BrowseEditFieldColor, BrowseEditDisabledFieldColor, BrowseEditTextColor,
    // BrowseEditDisabledTextColor, BrowseEditInvalidDropFieldColor
    static void applyFlattenedFieldPalette(const BrowseEdit* browseEdit, QLineEdit* lineEdit)
    {
        if (!StyleManager::stylesheetsDisabled() || !lineEdit)
        {
            return;
        }
        QColor field(0xCC, 0xCC, 0xCC);
        QColor disabledField(0x66, 0x66, 0x66);
        if (Style::hasClass(browseEdit, ValidDropTarget))
        {
            field = disabledField = Qt::transparent;
        }
        else if (Style::hasClass(browseEdit, InvalidDropTarget))
        {
            field = disabledField = QColor(0xFF, 0xFF, 0xFF);
        }
        QPalette fieldPalette = lineEdit->palette();
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            const bool disabled = group == QPalette::Disabled;
            QColor text = disabled ? QColor(0x99, 0x99, 0x99) : QColor(Qt::black);
            fieldPalette.setColor(group, QPalette::Base, disabled ? disabledField : field);
            fieldPalette.setColor(group, QPalette::Text, text);
            text.setAlpha((text.alpha() + 1) / 2);
            fieldPalette.setColor(group, QPalette::PlaceholderText, text);
        }
        lineEdit->setPalette(fieldPalette);
        lineEdit->update();
    }

    void BrowseEdit::applyDropTargetStyle(BrowseEdit* browseEdit, bool valid)
    {
        BrowseEdit::removeDropTargetStyle(browseEdit);
        Style::addClass(browseEdit, valid ? ValidDropTarget : InvalidDropTarget);
        StyleManager::repolishStyleSheet(browseEdit);
        applyFlattenedFieldPalette(browseEdit, browseEdit->m_data->m_lineEdit);
        browseEdit->update();
    }

    void BrowseEdit::removeDropTargetStyle(BrowseEdit* browseEdit)
    {
        Style::removeClass(browseEdit, ValidDropTarget);
        Style::removeClass(browseEdit, InvalidDropTarget);
        StyleManager::repolishStyleSheet(browseEdit);
        applyFlattenedFieldPalette(browseEdit, browseEdit->m_data->m_lineEdit);
        browseEdit->update();
    }

    bool BrowseEdit::drawFlattenedField(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        // The field of the line edit inside a BrowseEdit: the rule drew its background over the line edit's rect, inside the path of
        // its rounded left corners (BrowseEditFlat::fieldPath), and no border. The fill is the Base of the palette
        // (applyFlattenedFieldPalette: #CCCCCC, :disabled #666666, a valid drop target none, an invalid one #FFFFFF), except that the
        // field is #FFFFFF while it has the focus (BrowseEdit.qss:38-43 `QLineEdit:focus`), the valid drop target's transparent
        // rule (:31-36, one pseudo-class more) aside. UNVERIFIED against a capture: none holds a focused field (FieldBrowseEditTests holds
        // the focus and hover states against the sheet). TOKENIZE: BrowseEditFocusFieldColor
        const auto* lineEdit = qobject_cast<const QLineEdit*>(widget);
        if (!option || !lineEdit || !qobject_cast<const BrowseEdit*>(lineEdit->parentWidget()))
        {
            return false;
        }

        QBrush fill = option->palette.brush(QPalette::Base);
        const bool focused = option->state.testFlag(QStyle::State_Enabled) && option->state.testFlag(QStyle::State_HasFocus);
        if (focused && fill.style() != Qt::NoBrush && fill.color().alpha() != 0)
        {
            fill = QBrush(QColor(0xFF, 0xFF, 0xFF));
        }
        if (fill.style() == Qt::NoBrush || (fill.style() == Qt::SolidPattern && fill.color().alpha() == 0))
        {
            return true;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillPath(BrowseEditFlat::fieldPath(QRectF(option->rect)), fill);
        painter->restore();
        return true;
    }

    bool BrowseEdit::eventFilter(QObject* watched, QEvent* event)
    {
        if (isEnabled() && (m_data->m_lineEdit == watched))
        {
            switch (event->type())
            {
                case QEvent::MouseButtonDblClick:
                    if (isLineEditReadOnly())
                    {
                        Q_EMIT attachedButtonTriggered();
                        return true;
                    }
                    break;

                case QEvent::FocusIn:
                    update();
                    break;

                case QEvent::FocusOut:
                    update();
                    break;

                default:
                    break;
            }
        }

        return QFrame::eventFilter(watched, event);
    }

    bool BrowseEdit::event(QEvent* event)
    {
        switch (event->type())
        {
            case QEvent::ToolTip:
            {
                const auto he = static_cast<QHelpEvent*>(event);
                const QString tooltipText = hasAcceptableInput() ? toolTip() : m_data->m_errorToolTip;
                QToolTip::showText(he->globalPos(), tooltipText, this, QRect(), toolTipDuration());
                return true;
            }
            default:
                break;
        }
        return QFrame::event(event);
    }

    void BrowseEdit::setHasAcceptableInput(bool acceptable)
    {
        if (m_data->m_hasAcceptableInput != acceptable)
        {
            m_data->m_hasAcceptableInput = acceptable;
            emit hasAcceptableInputChanged(m_data->m_hasAcceptableInput);
            update();
        }
    }

    void BrowseEdit::onTextChanged(const QString& text)
    {
        setHasAcceptableInput(hasAcceptableInput());
        emit textChanged(text);
    }

    bool BrowseEdit::polish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig)
    {
        Q_UNUSED(config);
        auto browseEdit = qobject_cast<BrowseEdit*>(widget);

        if (browseEdit)
        {
            style->repolishOnSettingsChange(browseEdit);
            auto lineEdit = browseEdit->m_data->m_lineEdit;
            LineEdit::polish(style, lineEdit, lineEditConfig);

            applyFlattenedFieldPalette(browseEdit, lineEdit);

            QAction* action = lineEdit->findChild<QAction*>(ClearButtonActionNameC);
            if (action)
            {
                QStyleOptionFrame option;
                option.initFrom(lineEdit);
                action->setIcon(LineEdit::clearButtonIcon(&option, lineEdit, lineEditConfig));
            }
        }
        return browseEdit;
    }

    bool BrowseEdit::unpolish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig)
    {
        Q_UNUSED(config);
        auto browseEdit = qobject_cast<BrowseEdit*>(widget);

        if (browseEdit)
        {
            auto lineEdit = browseEdit->m_data->m_lineEdit;
            LineEdit::unpolish(style, lineEdit, lineEditConfig);

            QAction* action = lineEdit->findChild<QAction*>(ClearButtonActionNameC);
            if (action)
            {
                QStyleOptionFrame option;
                option.initFrom(lineEdit);
                action->setIcon(lineEdit->style()->standardIcon(QStyle::SP_LineEditClearButton, &option, lineEdit));
            }
        }
        return browseEdit;
    }

    bool BrowseEdit::drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const BrowseEdit::Config& config)
    {
        const auto browserEdit = qobject_cast<const BrowseEdit*>(widget);
        if (!browserEdit)
        {
            return false;
        }

        const bool validDropTarget = Style::hasClass(browserEdit, ValidDropTarget);
        const bool invalidDropTarget = Style::hasClass(browserEdit, InvalidDropTarget);
        const bool dropTarget = validDropTarget || invalidDropTarget;
        const bool hasError = !browserEdit->hasAcceptableInput();
        const int lineWidth = config.getLineWidth(option, hasError, dropTarget);

        // Flattened: the frame sits inside the BrowseEdit.qss 2px margin, i.e. on the frame
        // rect the style option carries (legacy capture 2026-09-24: 1px ring, not 3px).
        const QRect contentsRect = StyleManager::stylesheetsDisabled() ? option->rect : browserEdit->rect();

        if (lineWidth > 0)
        {
            const QColor frameColor = config.getBorderColor(option, hasError, dropTarget);
            const QPainterPath borderRect = style->borderLineEditRect(contentsRect, lineWidth, config.borderRadius);
            Style::drawFrame(painter, borderRect, Qt::NoPen, frameColor);
        }
        else
        {
            // Draw attached button border
            const int borderRadius = config.borderRadius;
            const int borderWidth = 1;

            const int borderAdjustment = borderRadius - borderWidth;
            const int radius = borderRadius + borderWidth;

            QRect buttonRect = browserEdit->m_data->m_attachedButton->rect();
            buttonRect.moveCenter(contentsRect.center());
            buttonRect.moveRight(contentsRect.right() - borderRadius);
            buttonRect.adjust(0, -borderAdjustment, borderAdjustment, borderAdjustment);

            const int diameter = radius * 2;
            const int adjustedDiamter = diameter - borderAdjustment;
            QRect tr(buttonRect.right() - adjustedDiamter, buttonRect.top(), diameter, diameter);
            QRect br(buttonRect.right() - adjustedDiamter, buttonRect.bottom() - adjustedDiamter, diameter, diameter);

            QPainterPath pathRect;
            pathRect.moveTo(buttonRect.topLeft());
            pathRect.lineTo(tr.topLeft());
            pathRect.arcTo(tr, 90, -90);
            pathRect.lineTo(br.right() + borderWidth, br.top());
            pathRect.arcTo(br, 0, -90);
            pathRect.lineTo(buttonRect.left(), buttonRect.bottom() + borderWidth);

            Style::drawFrame(painter, pathRect, Qt::NoPen, config.hoverBorderColor);
        }

        if (validDropTarget)
        {
            QRect buttonRect = browserEdit->m_data->m_attachedButton->rect();
            buttonRect.moveCenter(contentsRect.center());
            buttonRect.moveRight(contentsRect.right() - config.borderRadius);
            const int verticalAdjustment = 1;
            buttonRect.adjust(0, verticalAdjustment, 0, 0);

            // The sheet style handed this its option with the rect inside the margin of the BrowseEdit's rule (CE_ShapedFrame,
            // qstylesheetstyle.cpp:4502-4511: borderRect), so the frame lay 2px further in than the ring, which is drawn on the
            // whole rect (above). The flattened option holds the whole rect.
            const int offset = config.dropFrameOffset;
            const int margin = StyleManager::stylesheetsDisabled() ? BrowseEditFlat::Margin : 0;
            const QRect dropFrameRect = option->rect.adjusted(margin + offset, margin + offset, -(margin + offset), -(margin + offset));
            QPainterPath dropFramePath = style->lineEditRect(dropFrameRect, lineWidth, config.dropFrameRadius);
            dropFramePath.moveTo(buttonRect.topLeft());
            dropFramePath.lineTo(buttonRect.bottomLeft());

            Style::drawFrame(painter, dropFramePath, QPen(Qt::white), Qt::NoBrush);
        }

        return true;
    }

} // namespace AzQtComponents 

