/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphItemDelegate.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QIcon>
#include <QPainter>


namespace EMStudio
{
    AnimGraphItemDelegate::AnimGraphItemDelegate(QWidget* parent)
        : QStyledItemDelegate(parent)
    {
        m_referenceBackground = MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/ReferenceBackground.png").pixmap(QSize(1, 17));
        m_referenceArrow = MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/ReferenceArrow.png").pixmap(QSize(4, 17));
    }

    void AnimGraphItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        const AnimGraphModel::ModelItemType modelItemType = index.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
        if (modelItemType == AnimGraphModel::ModelItemType::NODE)
        {
            if (index.column() == AnimGraphModel::COLUMN_NAME)
            {
                const AZ::TypeId typeId = index.data(AnimGraphModel::ROLE_RTTI_TYPE_ID).value<AZ::TypeId>();
                if (azrtti_typeid<EMotionFX::AnimGraphReferenceNode>() == typeId)
                {
                    painter->save();
                    painter->setPen(Qt::NoPen);

                    painter->setBrush(m_referenceBackground);
                    const QRect backGroundRect(option.rect.left(), option.rect.top(), option.rect.width() - 4, option.rect.height());
                    painter->drawRect(backGroundRect);

                    const QRect arrowRect(option.rect.left() + option.rect.width() - 4, option.rect.top(), 4, option.rect.height());
                    painter->drawPixmap(arrowRect, m_referenceArrow);

                    painter->restore();
                }
            }

            QStyledItemDelegate::paint(painter, option, index);
        }
        else
        {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

    QSize AnimGraphItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);

        const AnimGraphModel::ModelItemType modelItemType = index.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
        if (modelItemType == AnimGraphModel::ModelItemType::NODE)
        {
            if (index.column() == AnimGraphModel::COLUMN_NAME)
            {
                const AZ::TypeId typeId = index.data(AnimGraphModel::ROLE_RTTI_TYPE_ID).value<AZ::TypeId>();
                if (azrtti_typeid<EMotionFX::AnimGraphReferenceNode>() == typeId)
                {
                    size.setWidth(size.width() + 10);
                }
            }
        }
        
        size.setHeight(17); // size of the background image for reference node
        return size;
    }

    void AnimGraphItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_AnimGraphItemDelegate.cpp>
