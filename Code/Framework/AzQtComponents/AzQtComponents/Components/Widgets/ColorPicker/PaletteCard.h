/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#endif

class QUndoStack;
class QMargins;
class QLayout;

namespace AzQtComponents
{
    namespace Internal
    {
        class ColorController;
    }

    class CardHeader;
    class PaletteView;

    class AZ_QT_COMPONENTS_API PaletteCardBase
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(QSharedPointer<Palette> palette READ palette)

    public:
        QSharedPointer<Palette> palette() const;

        void setSwatchSize(const QSize& size);
        void setGammaEnabled(bool enabled);
        void setGamma(qreal gamma);

        void tryAdd(const AZ::Color& color);
        bool contains(const AZ::Color& color) const;

        void setContentsMargins(const QMargins& margins);

    Q_SIGNALS:
        void contextMenuRequested(const QPoint& position);

    protected:
        explicit PaletteCardBase(QSharedPointer<Palette> palette, Internal::ColorController* controller, QUndoStack* undoStack, QWidget* parent = nullptr);

        bool m_modified;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QSharedPointer<Palette> m_palette;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

        CardHeader* m_header;
        PaletteView* m_paletteView;
        QLayout* m_contentsLayout;
    };

    class AZ_QT_COMPONENTS_API PaletteCard
        : public PaletteCardBase
    {
        Q_OBJECT
        Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
        Q_PROPERTY(bool modified READ modified WRITE setModified NOTIFY modifiedChanged)

    public:
        PaletteCard(QSharedPointer<Palette> palette, Internal::ColorController* controller, QUndoStack* undoStack, QWidget* parent = nullptr);
        ~PaletteCard() override;

        QString title() const;
        void setTitle(const QString& title);

        bool modified() const;
        void setModified(bool modified);

        void setExpanded(bool expanded);
        bool isExpanded() const;

    Q_SIGNALS:
        void titleChanged(const QString& title);
        void modifiedChanged(bool modified);
        void removeClicked();
        void saveClicked();
        void saveAsClicked();

    private Q_SLOTS:
        virtual void handleModelChanged();

    private:
        void updateHeader();

        QString m_title;
        bool m_modified = false;
    };

    class AZ_QT_COMPONENTS_API QuickPaletteCard
        : public PaletteCardBase
    {
        Q_OBJECT

    public:
        explicit QuickPaletteCard(QSharedPointer<Palette> palette, Internal::ColorController* controller, QUndoStack* undoStack, QWidget* parent = nullptr);

    Q_SIGNALS:

        void selectedColorsChanged(const QVector<AZ::Color>& selectedColors);
    };
} // namespace AzQtComponents
