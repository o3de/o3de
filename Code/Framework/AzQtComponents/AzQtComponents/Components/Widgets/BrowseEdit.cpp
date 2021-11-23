/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
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
#include <QStyleOptionFrame>

namespace AzQtComponents
{
    static const QString ClearButtonActionNameC = QStringLiteral("_q_qlineeditclearaction");

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
        AzQtComponents::Style::addClass(this, "AzQtComponentsBrowseEdit");
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
        LineEdit::setEnableClearButtonWhenReadOnly(m_data->m_lineEdit, true);
        boxLayout->addWidget(m_data->m_lineEdit);

        connect(m_data->m_lineEdit, &QLineEdit::returnPressed, this, &BrowseEdit::returnPressed);
        connect(m_data->m_lineEdit, &QLineEdit::editingFinished, this, &BrowseEdit::editingFinished);
        connect(m_data->m_lineEdit, &QLineEdit::textChanged, this, &BrowseEdit::onTextChanged);
        connect(m_data->m_lineEdit, &QLineEdit::textEdited, this, &BrowseEdit::textEdited);

        m_data->m_attachedButton = new QPushButton(this);
        m_data->m_attachedButton->setObjectName("attached-button");
        boxLayout->addWidget(m_data->m_attachedButton);
        connect(m_data->m_attachedButton, &QPushButton::clicked, this, &BrowseEdit::attachedButtonTriggered);
        setAttachedButtonIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));
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

    void BrowseEdit::applyDropTargetStyle(BrowseEdit* browseEdit, bool valid)
    {
        BrowseEdit::removeDropTargetStyle(browseEdit);
        Style::addClass(browseEdit, valid ? ValidDropTarget : InvalidDropTarget);
        StyleManager::repolishStyleSheet(browseEdit);
    }

    void BrowseEdit::removeDropTargetStyle(BrowseEdit* browseEdit)
    {
        Style::removeClass(browseEdit, ValidDropTarget);
        Style::removeClass(browseEdit, InvalidDropTarget);
        StyleManager::repolishStyleSheet(browseEdit);
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

        const QRect contentsRect = browserEdit->rect();

        if (lineWidth > 0)
        {
            const QColor frameColor = config.getBorderColor(option, hasError, dropTarget);
            const QPainterPath borderRect = style->borderLineEditRect(browserEdit->rect(), lineWidth, config.borderRadius);
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

            const int offset = config.dropFrameOffset;
            const QRect dropFrameRect = option->rect.adjusted(offset, offset, -offset, -offset);
            QPainterPath dropFramePath = style->lineEditRect(dropFrameRect, lineWidth, config.dropFrameRadius);
            dropFramePath.moveTo(buttonRect.topLeft());
            dropFramePath.lineTo(buttonRect.bottomLeft());

            Style::drawFrame(painter, dropFramePath, QPen(Qt::white), Qt::NoBrush);
        }

        return true;
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_BrowseEdit.cpp"
