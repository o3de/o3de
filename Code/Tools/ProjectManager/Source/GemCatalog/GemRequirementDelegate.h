/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GemCatalog/GemItemDelegate.h>


namespace O3DE::ProjectManager
{
    //! The paint values of the 'Manual setup is required' rows that the flattened GUI owns; the members of GemRequirementDelegate below
    //! take their value from here. A named namespace, not an anonymous one: a unity build joins the files of a target into one
    //! translation unit, where anonymous namespaces merge.
    namespace GemRequirementDelegateFlat
    {
        // ------------------------------------------------------------------------------
        // The rows of the requirements list (GemRequirementDelegate::paint)
        // ------------------------------------------------------------------------------
        inline constexpr QColor ListSurface(0x44, 0x44, 0x44);  // GemRequirementDelegate.h:28 (outside). TOKENIZE: GemRequirementSurface
        inline constexpr QColor ItemFill(0x39, 0x39, 0x39);     // GemRequirementDelegate.h:29 (item). TOKENIZE: GemRequirementItemFill
    } // namespace GemRequirementDelegateFlat

    class GemRequirementDelegate
        : public GemItemDelegate
    {
        Q_OBJECT

    public:
        explicit GemRequirementDelegate(QAbstractItemModel* model, QObject* parent = nullptr);
        ~GemRequirementDelegate() = default;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) override;

        const QColor m_backgroundColor = GemRequirementDelegateFlat::ListSurface; // Outside of the actual gem item
        const QColor m_itemBackgroundColor = GemRequirementDelegateFlat::ItemFill; // Background color of the gem item

    private:
        QRect CalcRequirementRect(const QRect& contentRect) const;
    };
} // namespace O3DE::ProjectManager
