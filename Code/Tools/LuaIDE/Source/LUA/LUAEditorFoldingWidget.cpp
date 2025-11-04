/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorFoldingWidget.hxx"

#include "LUAEditorBlockState.h"
#include "LUAEditorPlainTextEdit.hxx"
#include "LUAEditorStyleMessages.h"

#include <Source/LUA/moc_LUAEditorFoldingWidget.cpp>

#include <QTextBlock>
#include <QStyleOption>
#include <QStyle>
#include <QPainter>

namespace LUAEditor
{
    FoldingWidget::FoldingWidget(QWidget* pParent)
        : QWidget(pParent)
    {
        setEnabled(false);
    }

    FoldingWidget::~FoldingWidget()
    {
    }

    void FoldingWidget::paintEvent([[maybe_unused]] QPaintEvent* paintEvent)
    {
        auto colors = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        auto cursor = m_textEdit->textCursor();
        auto selectedBlock = m_textEdit->document()->findBlock(cursor.position());
        int startSelectedFold = 0;
        int endSelectedFold = 0;

        if (selectedBlock.isValid()) //detect if this block closes a fold
        {
            QTBlockState blockState;
            blockState.m_qtBlockState = selectedBlock.userState();
            auto prevBlock = selectedBlock.previous();
            if (!blockState.m_blockState.m_uninitialized && prevBlock.isValid())
            {
                QTBlockState prevBlockState;
                prevBlockState.m_qtBlockState = prevBlock.userState();
                if (blockState.m_blockState.m_foldLevel < prevBlockState.m_blockState.m_foldLevel)
                {
                    selectedBlock = prevBlock;
                }
            }
        }

        if (selectedBlock.isValid())
        {
            startSelectedFold = selectedBlock.blockNumber();
            endSelectedFold = selectedBlock.blockNumber() + 1;

            QTBlockState blockState;
            blockState.m_qtBlockState = selectedBlock.userState();
            if (!blockState.m_blockState.m_uninitialized && blockState.m_blockState.m_foldLevel > 0)
            {
                auto startBlock = selectedBlock;
                while (startBlock.isValid())
                {
                    auto prevBlock = startBlock.previous();
                    QTBlockState prevBlockState;
                    prevBlockState.m_qtBlockState = prevBlock.userState();

                    if (prevBlockState.m_blockState.m_foldLevel >= blockState.m_blockState.m_foldLevel)
                    {
                        startBlock = prevBlock;
                    }
                    else
                    {
                        break;
                    }
                }
                startSelectedFold = startBlock.blockNumber();

                auto endBlock = selectedBlock;
                while (endBlock.isValid())
                {
                    auto nextBlock = endBlock.next();
                    QTBlockState nextBlockState;
                    nextBlockState.m_qtBlockState = nextBlock.userState();

                    if (nextBlockState.m_blockState.m_foldLevel >= blockState.m_blockState.m_foldLevel)
                    {
                        endBlock = nextBlock;
                    }
                    else //less than, which is included in this fold
                    {
                        endBlock = nextBlock;
                        break;
                    }
                }
                endSelectedFold = endBlock.blockNumber() + 1;
            }
        }

        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        AZ::u32 lastFoldLevel = 0;
        bool firstBlock = true;
        m_textEdit->ForEachVisibleBlock([&](const QTextBlock& block, const QRectF& blockRect)
            {
                QTBlockState blockState;
                blockState.m_qtBlockState = block.userState();
                if (!blockState.m_blockState.m_uninitialized)
                {
                    if (firstBlock)
                    {
                        lastFoldLevel = 0;
                        auto prevBlock = block.previous();
                        if (prevBlock.isValid())
                        {
                            QTBlockState prevBlockState;
                            prevBlockState.m_qtBlockState = prevBlock.userState();
                            lastFoldLevel = prevBlockState.m_blockState.m_foldLevel;
                        }
                        firstBlock = false;
                    }
                    auto centerToBorder = (m_singleSize - 2 * c_borderSize) / 2;
                    QRect drawRect = blockRect.toRect();
                    drawRect.setLeft(0);
                    drawRect.setRight(m_singleSize);

                    auto oldPen = p.pen();
                    auto oldBrush = p.brush();

                    if (block.blockNumber() >= startSelectedFold && block.blockNumber() < endSelectedFold)
                    {
                        p.setPen(colors->GetFoldingSelectedColor());
                    }
                    else
                    {
                        p.setPen(colors->GetFoldingColor());
                    }
                    p.setBrush(QBrush(Qt::BrushStyle::NoBrush));
                    if (blockState.m_blockState.m_foldLevel > lastFoldLevel)
                    {
                        auto centerEdge = AZStd::max(m_singleSize / 10, 2); //for center lines, 10% of our size
                        auto center = drawRect.center();
                        QRect square(center.x() - centerToBorder, center.y() - centerToBorder, 2 * centerToBorder, 2 * centerToBorder);
                        p.drawRect(square);
                        p.drawLine(square.left() + centerEdge, center.y(), square.right() - centerEdge + 1, center.y());
                        if (blockState.m_blockState.m_folded)
                        {
                            p.drawLine(center.x(), square.top() + centerEdge, center.x(), square.bottom() - centerEdge + 1);
                        }
                        p.drawLine(center.x(), square.bottom() + 1, center.x(), drawRect.bottom());
                        if (blockState.m_blockState.m_foldLevel > 1)
                        {
                            if (block.blockNumber() == startSelectedFold)
                            {
                                p.setPen(colors->GetFoldingColor());
                            }
                            p.drawLine(center.x(), drawRect.top(), center.x(), square.top() - 1);
                        }
                    }
                    else if (blockState.m_blockState.m_foldLevel < lastFoldLevel)
                    {
                        auto center = drawRect.center();
                        p.drawLine(center.x(), drawRect.top(), center.x(), center.y());
                        p.drawLine(center.x(), center.y(), drawRect.right(), center.y());
                        if (blockState.m_blockState.m_foldLevel > 0)
                        {
                            if (block.blockNumber() == endSelectedFold - 1)
                            {
                                p.setPen(colors->GetFoldingColor());
                            }
                            p.drawLine(center.x(), center.y(), center.x(), drawRect.bottom());
                        }
                    }
                    else if (blockState.m_blockState.m_foldLevel > 0)
                    {
                        auto center = drawRect.center();
                        p.drawLine(center.x(), drawRect.top(), center.x(), drawRect.bottom());
                    }

                    lastFoldLevel = blockState.m_blockState.m_foldLevel;
                    p.setPen(oldPen);
                    p.setBrush(oldBrush);
                }
            });
    }

    void FoldingWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        auto mousePos = event->localPos();

        m_textEdit->ForEachVisibleBlock([&](QTextBlock& blockClicked, const QRectF& blockRect)
            {
                if (mousePos.y() >= blockRect.top() && mousePos.y() <= blockRect.bottom())
                {
                    auto prevBlock = blockClicked.previous();
                    QTBlockState state;
                    state.m_qtBlockState = blockClicked.userState();

                    AZ::u32 prevFoldLevel = 0;
                    if (prevBlock.isValid())
                    {
                        QTBlockState prevBlockState;
                        prevBlockState.m_qtBlockState = prevBlock.userState();
                        prevFoldLevel = prevBlockState.m_blockState.m_foldLevel;
                    }
                    if (!state.m_blockState.m_uninitialized && state.m_blockState.m_foldLevel > prevFoldLevel)
                    {
                        state.m_blockState.m_folded = 1 - state.m_blockState.m_folded;
                        blockClicked.setUserState(state.m_qtBlockState);

                        int startDirty = blockClicked.position();
                        int dirtyLength = (blockClicked.position() + blockClicked.length()) - startDirty;

                        auto nextBlock = blockClicked.next();
                        QTBlockState nextState;
                        nextState.m_qtBlockState = nextBlock.userState();
                        bool currentChildFolded = false;
                        AZ::u32 currentChildFoldLevel = state.m_blockState.m_foldLevel;
                        while (nextBlock.isValid() && nextState.m_blockState.m_foldLevel >= state.m_blockState.m_foldLevel)
                        {
                            if (state.m_blockState.m_folded)
                            {
                                nextBlock.setVisible(false);
                            }
                            else // we are unfolding, need to preserve any child folds that were already folded before we were
                            {
                                if (!currentChildFolded)
                                {
                                    nextBlock.setVisible(true);
                                    if (nextState.m_blockState.m_foldLevel > currentChildFoldLevel)
                                    {
                                        currentChildFoldLevel = nextState.m_blockState.m_foldLevel;
                                        if (nextState.m_blockState.m_folded)
                                        {
                                            currentChildFolded = true;
                                        }
                                    }
                                }
                                else
                                {
                                    bool visible = false;
                                    if (nextState.m_blockState.m_foldLevel < currentChildFoldLevel)
                                    {
                                        currentChildFolded = false;
                                        visible = true;
                                    }
                                    nextBlock.setVisible(visible);
                                }
                            }
                            dirtyLength = (nextBlock.position() + nextBlock.length()) - startDirty;


                            nextBlock = nextBlock.next();
                            nextState.m_qtBlockState = nextBlock.userState();
                        }

                        update();
                        m_textEdit->document()->markContentsDirty(startDirty, dirtyLength);
                        TextBlockFoldingChanged();
                    }
                }
            });

        event->accept();
    }

    void FoldingWidget::OnContentChanged(int from, int, int charsAdded)
    {
        auto startBlock = m_textEdit->document()->findBlock(from);
        if (!startBlock.isValid())
        {
            return;
        }

        auto endBlock = m_textEdit->document()->findBlock(from + charsAdded);
        if (!endBlock.isValid())
        {
            endBlock = startBlock;
        }

        auto prevBlock = startBlock.previous();
        bool atLeastOne = false;
        while (prevBlock.isValid() && !prevBlock.isVisible())
        {
            startBlock = prevBlock;
            prevBlock = prevBlock.previous();
            atLeastOne = true;
        }
        if (atLeastOne) //need to grab the opening fold
        {
            startBlock = prevBlock;
        }

        auto nextBlock = endBlock.next();
        while (nextBlock.isValid() && !nextBlock.isVisible())
        {
            endBlock = nextBlock;
            nextBlock = nextBlock.next();
        }

        int startDirty = startBlock.position();
        int dirtyLength = (endBlock.position() + endBlock.length()) - startDirty;
        while (startBlock.isValid() && startBlock.blockNumber() <= endBlock.blockNumber())
        {
            QTBlockState state;
            state.m_qtBlockState = startBlock.userState();
            if (state.m_blockState.m_folded)
            {
                state.m_blockState.m_folded = 0;
                startBlock.setUserState(state.m_qtBlockState);
            }
            startBlock.setVisible(true);

            startBlock = startBlock.next();
        }

        update();
        m_textEdit->document()->markContentsDirty(startDirty, dirtyLength);
        TextBlockFoldingChanged();
    }

    void FoldingWidget::FoldAll()
    {
        AZ::u32 lastFoldLevel = 0;
        for (auto block = m_textEdit->document()->begin(); block != m_textEdit->document()->end(); block = block.next())
        {
            block.setVisible(true);

            QTBlockState state;
            state.m_qtBlockState = block.userState();

            if (state.m_blockState.m_foldLevel > lastFoldLevel)
            {
                state.m_blockState.m_folded = 1;
                block.setUserState(state.m_qtBlockState);

                if (lastFoldLevel != 0)
                {
                    block.setVisible(false);
                }
            }
            else if (state.m_blockState.m_foldLevel > 0)
            {
                block.setVisible(false);
            }

            lastFoldLevel = state.m_blockState.m_foldLevel;
        }

        update();
        m_textEdit->document()->markContentsDirty(0, m_textEdit->document()->characterCount());
        TextBlockFoldingChanged();
    }

    void FoldingWidget::UnfoldAll()
    {
        for (auto block = m_textEdit->document()->begin(); block != m_textEdit->document()->end(); block = block.next())
        {
            block.setVisible(true);
            QTBlockState state;
            state.m_qtBlockState = block.userState();
            state.m_blockState.m_folded = 0;
            block.setUserState(state.m_qtBlockState);
        }

        update();
        m_textEdit->document()->markContentsDirty(0, m_textEdit->document()->characterCount());
        TextBlockFoldingChanged();
    }

    void FoldingWidget::SetFont(QFont font)
    {
        QFontMetrics metrics(font);
        m_singleSize = metrics.height();
        setFixedWidth(m_singleSize);
    }
}
