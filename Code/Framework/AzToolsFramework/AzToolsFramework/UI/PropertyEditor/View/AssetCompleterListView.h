/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QListView>
#include <QStyledItemDelegate>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    // =======================================================================
    // Paint Colours
    // =======================================================================

    //! The wash the asset completer popup paints under the row the pointer is on, named, so the legacy and the flattened GUI read one
    //! value. A named namespace, not an anonymous one: a header is read by many files, and a unity build joins files into one translation
    //! unit, where anonymous namespaces merge.
    namespace AssetCompleterListViewFlat
    {
        //! AssetCompleterListView.h:34. TOKENIZE: AssetCompleterHoverWash
        inline constexpr QColor HoverWash(255, 255, 255, 30);
    }

    //! Delegate on the asset autocompleter that enables html styling (used to show highlight of searched word)
    class AZTF_API AssetCompleterDelegate : public QStyledItemDelegate
    {
    public:
        explicit AssetCompleterDelegate(QObject *parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

    private:
        static const int s_rowHeight = 20;
        inline static const QColor m_hoverColor = AssetCompleterListViewFlat::HoverWash;
    };

    //! List View of suggestions in the Asset Autocompleter for PropertyAssetCtrl
    class AZTF_API AssetCompleterListView
        : public QListView
    {
        Q_OBJECT

    public:
        explicit AssetCompleterListView(QWidget* parent);
        ~AssetCompleterListView();

        void SelectFirstItem();

    private:
        AssetCompleterDelegate* m_delegate;
    };

}
