/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/conversions.h>
#include "LUAEditorBreakpointWidget.hxx"
#include "LUAEditorPlainTextEdit.hxx"
#include "LUAEditorStyleMessages.h"

#include <Source/LUA/moc_LUAEditorBreakpointWidget.cpp>

#include <QTextBlockFormat>
#include <QTextBlock>
#include <QTextBlockUserData>
#include <QPainter>

namespace LUAEditor
{
    namespace
    {
        //this used to detect deleted lines, doesnt work for last line though, but that can be detected elsewhere.
        class OriginalLineNumber
            : public QTextBlockUserData
        {
        public:
            AZ_CLASS_ALLOCATOR(OriginalLineNumber, AZ::SystemAllocator);
            OriginalLineNumber(int lineNumber, AZStd::function<void(int)> callback)
                : m_Callback(callback)
                , m_LineNumber(lineNumber) {}
            virtual ~OriginalLineNumber()
            {
                if (m_Callback)
                {
                    m_Callback(m_LineNumber);
                }
            }
            int GetLineNumber() const { return m_LineNumber; }
            void CancelLineDelete() { m_Callback.clear(); }
        private:
            AZStd::function<void(int)> m_Callback;
            int m_LineNumber;
        };
    }

    LUAEditorBreakpointWidget::LUAEditorBreakpointWidget(QWidget* pParent)
        : QWidget(pParent)
        , m_textEdit(nullptr)
        , m_currentExecLine(-1)
    {
        setEnabled(false);
    }

    LUAEditorBreakpointWidget::~LUAEditorBreakpointWidget()
    {
    }

    void LUAEditorBreakpointWidget::PreDestruction()
    {
        ClearBreakpoints();
        m_textEdit = nullptr;
    }

    void LUAEditorBreakpointWidget::paintEvent([[maybe_unused]] QPaintEvent* paintEvent)
    {
        QPainter p(this);

        auto colors = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        auto oldPen = p.pen();
        auto oldBrush = p.brush();
        p.setFont(m_font);

        if (isEnabled())
        {
            p.fillRect(geometry(), colors->GetBreakpointFocusedBackgroundColor());
        }
        else
        {
            p.fillRect(geometry(), colors->GetBreakpointUnfocusedBackgroundColor());
        }

        QFontMetrics metrics(m_font);
        int avgCharWidth = metrics.averageCharWidth();

        m_textEdit->ForEachVisibleBlock([&](const QTextBlock& block, const QRectF& blockRect)
            {
                int lineNum = block.blockNumber() + 1; // offset by one because line number starts from 1
                AZStd::string lineNumStr;
                AZStd::to_string(lineNumStr, lineNum);
                QRectF drawRect = blockRect;
                drawRect.setLeft(c_borderSize);
                drawRect.setRight(c_borderSize + m_numDigits * avgCharWidth);

                p.setPen(colors->GetLineNumberColor());
                p.drawText(drawRect.toRect(), Qt::AlignRight | Qt::AlignBottom, lineNumStr.c_str());

                {
                    auto centerY = (drawRect.top() + metrics.leading() + drawRect.bottom()) / 2;
                    auto centerX = (2 * drawRect.right() + avgCharWidth + c_borderSize) / 2;
                    drawRect.setTop(centerY - (avgCharWidth / 2));
                    drawRect.setBottom(centerY + (avgCharWidth / 2));
                    drawRect.setLeft(centerX - (avgCharWidth / 2));
                    drawRect.setRight(centerX + (avgCharWidth / 2));
                }

                //breakpoint red dot
                if (m_breakpoints.find(lineNum) != m_breakpoints.end())
                {
                    p.setPen(QColor::fromRgb(255, 0, 0));
                    p.setBrush(QBrush(QColor::fromRgb(255, 0, 0)));
                    p.drawEllipse(drawRect);
                }

                //yellow triangle for currently executing line
                if (m_currentExecLine == lineNum)
                {
                    const QPointF marker[] = {
                        {drawRect.right(), drawRect.center().y()},
                        {drawRect.center().x(), drawRect.top() + 1},
                        {drawRect.center().x(), drawRect.bottom() - 1}
                    };

                    p.setPen(QColor::fromRgb(255, 255, 0));
                    p.setBrush(QBrush(QColor::fromRgb(255, 255, 0)));
                    p.drawPolygon(marker, sizeof(marker) / sizeof(QPointF));
                }
            });
        p.setPen(oldPen);
        p.setBrush(oldBrush);
    }

    void LUAEditorBreakpointWidget::AddBreakpoint(int lineNumber)
    {
        auto block = m_textEdit->document()->findBlockByNumber(lineNumber);
        if (block.isValid())
        {
            if (block.userData() != nullptr)
            {
                ((OriginalLineNumber*)block.userData())->CancelLineDelete();
            }
            block.setUserData(aznew OriginalLineNumber(lineNumber, [&](int lineNumber){ m_DeletedBreakpoints.push_back(lineNumber); }));
            m_breakpoints.insert(lineNumber);
            update();
        }
    }

    void LUAEditorBreakpointWidget::RemoveBreakpoint(int lineNumber)
    {
        auto numRemoved = m_breakpoints.erase(lineNumber);
        if (numRemoved != 0)
        {
            auto block = m_textEdit->document()->findBlockByNumber(lineNumber);
            if (block.isValid())
            {
                if (block.userData() != nullptr)
                {
                    ((OriginalLineNumber*)block.userData())->CancelLineDelete();
                }
                block.setUserData(nullptr);
            }
        }
        update();
    }

    bool LUAEditorBreakpointWidget::HasBreakpoint(int lineNumber) const
    {
        return m_breakpoints.find(lineNumber) != m_breakpoints.end();
    }

    void LUAEditorBreakpointWidget::ClearBreakpoints()
    {
        for (auto& breakpoint : m_breakpoints)
        {
            auto block = m_textEdit->document()->findBlockByNumber(breakpoint);
            if (block.isValid())
            {
                if (block.userData() != nullptr)
                {
                    ((OriginalLineNumber*)block.userData())->CancelLineDelete();
                }
                block.setUserData(nullptr);
            }
        }
        m_breakpoints.clear();
        update();
    }

    void LUAEditorBreakpointWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        auto mousePos = event->localPos();
        m_textEdit->ForEachVisibleBlock(
            [&](const QTextBlock& block, const QRectF& blockRect)
            {
                if (mousePos.y() >= blockRect.top() && mousePos.y() <= blockRect.bottom())
                {
                    emit toggleBreakpoint(block.blockNumber() + 1); // offset by one because line number starts from 1
                }
            });

        event->accept();
    }

    void LUAEditorBreakpointWidget::OnBlockCountChange()
    {
        if (!m_textEdit)
        {
            return;
        }

        AZStd::vector<AZStd::pair<int, int> > movedBreakpoints;

        for (auto block = m_textEdit->document()->begin(); block != m_textEdit->document()->end(); block = block.next())
        {
            if (block.userData() != nullptr)
            {
                if (block.blockNumber() != ((OriginalLineNumber*)block.userData())->GetLineNumber())
                {
                    movedBreakpoints.push_back({ ((OriginalLineNumber*)block.userData())->GetLineNumber(), block.blockNumber() });
                }
            }
            if (block.userData() != nullptr)
            {
                ((OriginalLineNumber*)block.userData())->CancelLineDelete();
            }
            block.setUserData(nullptr);
        }
        for (auto& breakpoint : m_breakpoints)
        {
            auto block = m_textEdit->document()->findBlockByNumber(breakpoint);
            if (block.isValid())
            {
                block.setUserData(aznew OriginalLineNumber(block.blockNumber(), [&](int lineNumber){ m_DeletedBreakpoints.push_back(lineNumber); }));
            }
        }
        for (auto& moves : movedBreakpoints)
        {
            emit breakpointLineMove(moves.first, moves.second);
        }
        for (auto& deletes : m_DeletedBreakpoints)
        {
            emit breakpointDelete(deletes);
        }
        m_DeletedBreakpoints.clear();
        UpdateSize();
        update();
    }

    void LUAEditorBreakpointWidget::OnCharsRemoved(int position, int charsRemoved)
    {
        if (!m_textEdit)
        {
            return;
        }

        if (charsRemoved <= 0)
        {
            return;
        }

        auto block = m_textEdit->document()->findBlock(position);
        if (block.userData() && block.blockNumber() != ((OriginalLineNumber*)block.userData())->GetLineNumber())
        {
            m_DeletedBreakpoints.push_back(((OriginalLineNumber*)block.userData())->GetLineNumber());
            ((OriginalLineNumber*)block.userData())->CancelLineDelete();
            block.setUserData(nullptr);
        }
    }

    void LUAEditorBreakpointWidget::SetFont(QFont font)
    {
        m_font = font;
        UpdateSize();
    }

    void LUAEditorBreakpointWidget::UpdateSize()
    {
        QFontMetrics metrics(m_font);
        auto blockCount = m_textEdit->document()->blockCount();
        m_numDigits = 0;
        while (blockCount > 0)
        {
            ++m_numDigits;
            blockCount /= 10;
        }
        setFixedWidth(metrics.averageCharWidth() * (m_numDigits + 1) + 2 * c_borderSize); //+1 for the breakpoint
    }
}
