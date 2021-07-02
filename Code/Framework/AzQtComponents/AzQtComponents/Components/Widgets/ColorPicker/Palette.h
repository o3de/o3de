/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzCore/Math/Color.h>
#include <QMetaType>
#include <QVector>

class QDataStream;
class QString;
class QIODevice;
class QMimeData;

namespace AzQtComponents
{

    static const QString MIME_TYPE_PALETTE = QStringLiteral("application/x-o3de-color-palette+xml");

    class PaletteModel;

    /*!
     * A collection of colors and associated information.
     * Duplicate colors are not permitted within a palette, with comparison between colors being
     * defined by \ref AZ::Color::IsClose.
     */
    class AZ_QT_COMPONENTS_API Palette

    {
    public:
        Palette() = default;
        Palette(const Palette&) = default;
        Palette(Palette&&) = default;
        Palette(const QVector<AZ::Color>& colors);

        Palette& operator=(const Palette& other) = default;

        bool save(const QString& fileName) const;
        bool save(QIODevice* device) const;
        bool save(QMimeData* mimeData) const;

        bool load(const QString& fileName);
        bool load(QIODevice* device);
        bool load(const QMimeData* mimeData);

        /*!
         * Attempt to insert a color at the specified position.
         * Attempts to insert duplicate colors (according to \ref AZ::Color::IsClose) will be rejected.
         */
        bool tryInsertColor(int index, const AZ::Color& color);
        /*!
         * Attempt to insert several colors at the specified position.
         * Attempts to insert a duplicate color (according to \ref AZ::Color::IsClose) will cause the
         * entire insertion to be rejected.
         */
        bool tryInsertColors(int index, const QVector<AZ::Color>& colors);
        /*!
         * Attempt to insert several colors at the specified position.
         * Attempts to insert a duplicate color (according to \ref AZ::Color::IsClose) will cause the
         * entire insertion to be rejected.
         */
        bool tryInsertColors(int index, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last);

        /*!
         * Attempt to append a color to the palette.
         * Attempts to append a duplicate color (according to \ref AZ::Color::IsClose) will be rejected.
         */
        bool tryAppendColor(const AZ::Color& color);
        /*!
         * Attempt to append several colors to the palette.
         * Attempts to append a duplicate color (according to \ref AZ::Color::IsClose) will cause the
         * entire append to be rejected.
         */
        bool tryAppendColors(const QVector<AZ::Color>& colors);
        /*!
         * Attempt to append several colors to the palette.
         * Attempts to append a duplicate color (according to \ref AZ::Color::IsClose) will cause the
         * entire append to be rejected.
         */
        bool tryAppendColors(QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last);

        /*!
         * Remove colors from the palette.
         * The color at \ref index will be removed, along with \ref count - 1 that follow it. All
         * indices must be valid in order for the removal to succeed.
         */
        bool tryRemoveColors(int index, int count = 1);

        /*
         * Attempt to update the color at the specified index.
         * If the update would result in a duplicate color within the palette, it will be rejected.
         */
        bool trySetColor(int index, const AZ::Color& color);
    
        //! Check whether a color is present in the palette.
        bool containsColor(const AZ::Color& color);
        //! Check whether any of the specified colors are present in the palette.
        bool containsAnyColor(const QVector<AZ::Color>& colors);
        //! Check whether any of the specified colors are present in the palette.
        bool containsAnyColor(QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last);

        //! Check whether the palette is valid
        //! Currently this is only a check for duplicate colors.
        bool isValid() const;

        const QVector<AZ::Color>& colors() const;

    protected:
        friend class PaletteModel;
        friend QDataStream& operator<<(QDataStream& out, const Palette& palette);
        friend QDataStream& operator>>(QDataStream& in, Palette& palette);

        //! Used by PaletteModel to insert potentially duplicate colors during drag & drop operations
        //! The reason is that the model sees two changes for an internal drag that repositions items:
        //! The first is a removal of the moved items and the second is the insertion of them in their
        //! new location.
        void insertColorsIgnoringDuplicates(int index, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last);

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::Palette::m_colors': class 'QVector<AZ::Color>' needs to have dll-interface to be used by clients of class 'AzQtComponents::Palette'
        QVector<AZ::Color> m_colors;
        AZ_POP_DISABLE_WARNING
    };

    QDataStream& operator<<(QDataStream& out, const Palette& palette);
    QDataStream& operator>>(QDataStream& in, Palette& palette);
} // namespace AzQtComponents


Q_DECLARE_METATYPE(AZ::Color);
Q_DECLARE_METATYPE(AzQtComponents::Palette)

namespace AzQtComponents
{
    inline void registerMetaTypes()
    {
        qRegisterMetaType<AZ::Color>();
        qRegisterMetaType<AzQtComponents::Palette>();
    }
}

