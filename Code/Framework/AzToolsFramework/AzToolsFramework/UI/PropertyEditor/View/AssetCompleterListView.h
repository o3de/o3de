/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QListView>
#include <QStyledItemDelegate>
#endif
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    //! Delegate on the asset autocompleter that enables html styling (used to show highlight of searched word)
    class AssetCompleterDelegate : public QStyledItemDelegate
    {
    public:
        explicit AssetCompleterDelegate(QObject *parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

    private:
        static const int s_rowHeight = 20;
    };

    //! List View of suggestions in the Asset Autocompleter for PropertyAssetCtrl
    class AssetCompleterListView
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
