/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/ColorLabel.h>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorPreview.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCard.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QFile>
#include <QFrame>
#include <QGuiApplication>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSettings>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QVBoxLayout>

#include <memory>
#include <string>
#include <vector>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The Global box elements of the color picker and the color label (ColorPickerElements.cpp) against the legacy rules they carry
    // (ColorPicker.qss, ColorLabel.qss), and against what n2 (the legacy GUI as captured, Gallery 06_color-label and
    // 07_color-picker__dialog0) drew. The legacy twin of a widget is Qt's own style sheet style over Fusion with the application's real
    // sheet (BaseStyleSheet.qss and the files it imports, from the library's resources); the flat twin is the flattened Style with the
    // registered Global elements. Same box, same hints, same pixels. Pictures are what a window paints (QWidget::render) into an image
    // of an explicit device ratio, the ratios of the captures (1.5) and the whole ones.
    // ----------------------------------------------------------------------------------

    namespace ColorPickerElementTestData
    {
        //! A device pixel of an n2 capture: its window coordinates (the origin is the window's) and its color
        struct Sample
        {
            int m_x;
            int m_y;
            unsigned m_rgb;
        };

        //! A widget of a class that needs no more than its name here (a color picker, a card header, the sliders of a tab): a QFrame
        //! that answers inherits() for the class the matchers ask. The legacy sheet names it by a type selector instead (appSheet).
        class PickerStandIn
            : public QFrame
        {
        public:
            PickerStandIn(const char* className, const char* objectName, QWidget* parent)
                : QFrame(parent)
                , m_className(className)
            {
                setObjectName(QString::fromLatin1(objectName));
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? static_cast<void*>(this) : QFrame::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        const char* const PickerClass = "AzQtComponents::ColorPicker";
        const char* const CardHeaderClass = "AzQtComponents::CardHeader";
        const char* const HslSlidersClass = "AzQtComponents::HSLSliders";
        const char* const HsvSlidersClass = "AzQtComponents::HSVSliders";

        const char* const PickerSelector = "AzQtComponents--ColorPicker";
        const char* const CardHeaderSelector = "AzQtComponents--CardHeader";
        const char* const HslSlidersSelector = "AzQtComponents--HSLSliders";
        const char* const HsvSlidersSelector = "AzQtComponents--HSVSliders";

        //! What StyleSheetCache makes of BaseStyleSheet.qss: each @import line replaced by the file it names, from the real resources.
        //! The classes of the widgets the twin stands in for (the selectors named) become a type selector with the specificity of a
        //! class selector's, QFrame (what the stand-in is, and all that sits in its tree). The rest of the sheet is as it is: the other
        //! classes are real here, or not there at all.
        QString appSheet(std::initializer_list<const char*> standIns = {})
        {
            static QString expanded;
            if (expanded.isEmpty())
            {
                const auto read = [](const QString& name)
                {
                    QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + name);
                    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name.toUtf8().constData();
                    return QString::fromUtf8(file.readAll()).replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
                };
                expanded = read(QStringLiteral("BaseStyleSheet.qss"));
                const QRegularExpression import(QStringLiteral("@import\\s+\"([^\"]+)\"\\s*;"));
                for (QRegularExpressionMatch match = import.match(expanded); match.hasMatch(); match = import.match(expanded))
                {
                    expanded.replace(match.capturedStart(), match.capturedLength(), read(match.captured(1)));
                }
            }
            QString sheet = expanded;
            for (const char* selector : standIns)
            {
                // The picker's own rule (`padding: 100px`, ColorPicker.qss:11-14) is left out: a QWidget ignores it, a QFrame would not
                if (qstrcmp(selector, PickerSelector) == 0)
                {
                    sheet.replace(QStringLiteral("AzQtComponents--ColorPicker\n{\n    padding: 100px;"),
                        QStringLiteral("NoSuchType\n{\n    padding: 100px;"));
                }
                sheet.replace(QString::fromLatin1(selector), QStringLiteral("QFrame"));
            }
            return sheet;
        }

        // ------------------------------------------------------------------------------
        // What n2 drew, device pixel by device pixel (tools/gen_samples.py reads them from the capture): what is only the border and the
        // page behind it, at the corners and in the middle of each side. The widgets stand where n2 had them, in window coordinates.
        // ------------------------------------------------------------------------------

        //! ColorPreview 218 x 22 at (43, 206) of n2 07_color-picker__dialog0
        const Sample PreviewRing[] = {
            { 64, 308, 0x444444 }, { 65, 308, 0x444444 }, { 66, 308, 0x444444 }, { 67, 308, 0x444444 }, { 68, 308, 0x444444 },
            { 388, 308, 0x444444 }, { 389, 308, 0x444444 }, { 390, 308, 0x444444 }, { 391, 308, 0x444444 },
            { 392, 308, 0x444444 }, { 228, 308, 0x444444 }, { 64, 309, 0x444444 }, { 65, 309, 0x383838 },
            { 66, 309, 0x343434 }, { 67, 309, 0x333333 }, { 68, 309, 0x333333 }, { 388, 309, 0x333333 },
            { 389, 309, 0x343434 }, { 390, 309, 0x383838 }, { 391, 309, 0x444444 }, { 392, 309, 0x444444 },
            { 228, 309, 0x333333 }, { 64, 310, 0x444444 }, { 65, 310, 0x333333 }, { 66, 310, 0x333333 }, { 67, 310, 0x353535 },
            { 68, 310, 0x353535 }, { 388, 310, 0x353535 }, { 389, 310, 0x333333 }, { 390, 310, 0x333333 },
            { 391, 310, 0x3F3F3F }, { 392, 310, 0x444444 }, { 228, 310, 0x373737 }, { 64, 341, 0x444444 },
            { 65, 341, 0x383838 }, { 66, 341, 0x343434 }, { 67, 341, 0x333333 }, { 68, 341, 0x333333 }, { 388, 341, 0x333333 },
            { 389, 341, 0x343434 }, { 390, 341, 0x393939 }, { 391, 341, 0x444444 }, { 392, 341, 0x444444 },
            { 228, 341, 0x333333 }, { 64, 342, 0x444444 }, { 65, 342, 0x444444 }, { 66, 342, 0x444444 }, { 67, 342, 0x444444 },
            { 68, 342, 0x444444 }, { 388, 342, 0x444444 }, { 389, 342, 0x444444 }, { 390, 342, 0x444444 },
            { 391, 342, 0x444444 }, { 392, 342, 0x444444 }, { 228, 342, 0x444444 }, { 64, 311, 0x444444 },
            { 64, 312, 0x444444 }, { 64, 313, 0x444444 }, { 64, 314, 0x444444 }, { 64, 336, 0x444444 }, { 64, 337, 0x444444 },
            { 64, 338, 0x444444 }, { 64, 339, 0x444444 }, { 64, 325, 0x444444 }, { 65, 311, 0x333333 }, { 65, 312, 0x333333 },
            { 65, 313, 0x333333 }, { 65, 314, 0x333333 }, { 65, 336, 0x333333 }, { 65, 337, 0x333333 }, { 65, 338, 0x333333 },
            { 65, 339, 0x333333 }, { 65, 325, 0x333333 }, { 390, 311, 0x333333 }, { 390, 312, 0x333333 },
            { 390, 313, 0x333333 }, { 390, 314, 0x333333 }, { 390, 336, 0x333333 }, { 390, 337, 0x333333 },
            { 390, 338, 0x333333 }, { 390, 339, 0x333333 }, { 390, 325, 0x333333 }, { 391, 311, 0x393939 },
            { 391, 312, 0x343434 }, { 391, 313, 0x373737 }, { 391, 314, 0x373737 }, { 391, 336, 0x373737 },
            { 391, 337, 0x373737 }, { 391, 338, 0x343434 }, { 391, 339, 0x393939 }, { 391, 325, 0x373737 },
            { 392, 311, 0x444444 }, { 392, 312, 0x444444 }, { 392, 313, 0x444444 }, { 392, 314, 0x444444 },
            { 392, 336, 0x444444 }, { 392, 337, 0x444444 }, { 392, 338, 0x444444 }, { 392, 339, 0x444444 },
            { 392, 325, 0x444444 },
        };

        //! ColorGrid 218 x 150 at (43, 48)
        const Sample GridRing[] = {
            { 64, 71, 0x444444 }, { 65, 71, 0x444444 }, { 66, 71, 0x444444 }, { 67, 71, 0x444444 }, { 68, 71, 0x444444 },
            { 388, 71, 0x444444 }, { 389, 71, 0x444444 }, { 390, 71, 0x444444 }, { 391, 71, 0x444444 }, { 392, 71, 0x444444 },
            { 228, 71, 0x444444 }, { 64, 72, 0x444444 }, { 65, 72, 0x383838 }, { 66, 72, 0x343434 }, { 67, 72, 0x333333 },
            { 68, 72, 0x333333 }, { 388, 72, 0x333333 }, { 389, 72, 0x343434 }, { 390, 72, 0x383838 }, { 391, 72, 0x444444 },
            { 392, 72, 0x444444 }, { 228, 72, 0x333333 }, { 64, 73, 0x444444 }, { 65, 73, 0x333333 }, { 66, 73, 0x333333 },
            { 67, 73, 0x353535 }, { 68, 73, 0x353535 }, { 388, 73, 0x353535 }, { 389, 73, 0x333333 }, { 390, 73, 0x333333 },
            { 391, 73, 0x3F3F3F }, { 392, 73, 0x444444 }, { 228, 73, 0x373737 }, { 64, 296, 0x444444 }, { 65, 296, 0x383838 },
            { 66, 296, 0x343434 }, { 67, 296, 0x333333 }, { 68, 296, 0x333333 }, { 388, 296, 0x333333 },
            { 389, 296, 0x343434 }, { 390, 296, 0x393939 }, { 391, 296, 0x444444 }, { 392, 296, 0x444444 },
            { 228, 296, 0x333333 }, { 64, 297, 0x444444 }, { 65, 297, 0x444444 }, { 66, 297, 0x444444 }, { 67, 297, 0x444444 },
            { 68, 297, 0x444444 }, { 388, 297, 0x444444 }, { 389, 297, 0x444444 }, { 390, 297, 0x444444 },
            { 391, 297, 0x444444 }, { 392, 297, 0x444444 }, { 228, 297, 0x444444 }, { 64, 74, 0x444444 }, { 64, 75, 0x444444 },
            { 64, 76, 0x444444 }, { 64, 77, 0x444444 }, { 64, 291, 0x444444 }, { 64, 292, 0x444444 }, { 64, 293, 0x444444 },
            { 64, 294, 0x444444 }, { 64, 184, 0x444444 }, { 65, 74, 0x333333 }, { 65, 75, 0x333333 }, { 65, 76, 0x333333 },
            { 65, 77, 0x333333 }, { 65, 291, 0x333333 }, { 65, 292, 0x333333 }, { 65, 293, 0x333333 }, { 65, 294, 0x333333 },
            { 65, 184, 0x333333 }, { 390, 74, 0x333333 }, { 390, 75, 0x333333 }, { 390, 76, 0x333333 }, { 390, 77, 0x333333 },
            { 390, 291, 0x333333 }, { 390, 292, 0x333333 }, { 390, 293, 0x333333 }, { 390, 294, 0x333333 },
            { 390, 184, 0x333333 }, { 391, 74, 0x393939 }, { 391, 75, 0x343434 }, { 391, 76, 0x373737 }, { 391, 77, 0x373737 },
            { 391, 291, 0x373737 }, { 391, 292, 0x373737 }, { 391, 293, 0x343434 }, { 391, 294, 0x393939 },
            { 391, 184, 0x373737 }, { 392, 74, 0x444444 }, { 392, 75, 0x444444 }, { 392, 76, 0x444444 }, { 392, 77, 0x444444 },
            { 392, 291, 0x444444 }, { 392, 292, 0x444444 }, { 392, 293, 0x444444 }, { 392, 294, 0x444444 },
            { 392, 184, 0x444444 },
        };

        //! the add button 16 x 16 at (237, 511)
        const Sample AddSwatchRing[] = {
            { 355, 766, 0x444444 }, { 356, 766, 0x444444 }, { 357, 766, 0x444444 }, { 358, 766, 0x444444 },
            { 359, 766, 0x444444 }, { 376, 766, 0x444444 }, { 377, 766, 0x444444 }, { 378, 766, 0x444444 },
            { 379, 766, 0x444444 }, { 380, 766, 0x444444 }, { 368, 766, 0x444444 }, { 355, 767, 0x444444 },
            { 356, 767, 0x353535 }, { 357, 767, 0x333333 }, { 358, 767, 0x333333 }, { 359, 767, 0x333333 },
            { 376, 767, 0x333333 }, { 377, 767, 0x333333 }, { 378, 767, 0x353535 }, { 379, 767, 0x434343 },
            { 380, 767, 0x444444 }, { 368, 767, 0x333333 }, { 355, 768, 0x444444 }, { 356, 768, 0x333333 },
            { 357, 768, 0x383838 }, { 358, 768, 0x444444 }, { 359, 768, 0x444444 }, { 376, 768, 0x444444 },
            { 377, 768, 0x383838 }, { 378, 768, 0x333333 }, { 379, 768, 0x3F3F3F }, { 380, 768, 0x444444 },
            { 368, 768, 0x444444 }, { 355, 790, 0x444444 }, { 356, 790, 0x434343 }, { 357, 790, 0x3F3F3F },
            { 358, 790, 0x393939 }, { 359, 790, 0x3A3A3A }, { 376, 790, 0x393939 }, { 377, 790, 0x3F3F3F },
            { 378, 790, 0x434343 }, { 379, 790, 0x444444 }, { 380, 790, 0x444444 }, { 368, 790, 0x3B3B3B },
            { 355, 791, 0x444444 }, { 356, 791, 0x444444 }, { 357, 791, 0x444444 }, { 358, 791, 0x444444 },
            { 359, 791, 0x444444 }, { 376, 791, 0x444444 }, { 377, 791, 0x444444 }, { 378, 791, 0x444444 },
            { 379, 791, 0x444444 }, { 380, 791, 0x444444 }, { 368, 791, 0x444444 }, { 355, 769, 0x444444 },
            { 355, 770, 0x444444 }, { 355, 771, 0x444444 }, { 355, 772, 0x444444 }, { 355, 785, 0x444444 },
            { 355, 786, 0x444444 }, { 355, 787, 0x444444 }, { 355, 788, 0x444444 }, { 355, 779, 0x444444 },
            { 356, 769, 0x333333 }, { 356, 770, 0x333333 }, { 356, 771, 0x333333 }, { 356, 772, 0x333333 },
            { 356, 785, 0x333333 }, { 356, 786, 0x333333 }, { 356, 787, 0x333333 }, { 356, 788, 0x333333 },
            { 356, 779, 0x333333 }, { 378, 769, 0x333333 }, { 378, 770, 0x333333 }, { 378, 771, 0x333333 },
            { 378, 772, 0x333333 }, { 378, 785, 0x333333 }, { 378, 786, 0x333333 }, { 378, 787, 0x333333 },
            { 378, 788, 0x333333 }, { 378, 779, 0x333333 }, { 379, 769, 0x393939 }, { 379, 770, 0x3A3A3A },
            { 379, 771, 0x3B3B3B }, { 379, 772, 0x3B3B3B }, { 379, 785, 0x3B3B3B }, { 379, 786, 0x3A3A3A },
            { 379, 787, 0x393939 }, { 379, 788, 0x3F3F3F }, { 379, 779, 0x3B3B3B }, { 380, 769, 0x444444 },
            { 380, 770, 0x444444 }, { 380, 771, 0x444444 }, { 380, 772, 0x444444 }, { 380, 785, 0x444444 },
            { 380, 786, 0x444444 }, { 380, 787, 0x444444 }, { 380, 788, 0x444444 }, { 380, 779, 0x444444 },
        };

        //! the first swatch of the quick palette, 16 x 16 at (17, 511), #BF2828
        const Sample SwatchRing[] = {
            { 25, 766, 0x444444 }, { 26, 766, 0x444444 }, { 27, 766, 0x444444 }, { 28, 766, 0x444444 }, { 29, 766, 0x444444 },
            { 46, 766, 0x444444 }, { 47, 766, 0x444444 }, { 48, 766, 0x444444 }, { 49, 766, 0x444444 }, { 50, 766, 0x444444 },
            { 38, 766, 0x444444 }, { 25, 767, 0x444444 }, { 26, 767, 0x353535 }, { 27, 767, 0x333333 }, { 28, 767, 0x333333 },
            { 29, 767, 0x333333 }, { 46, 767, 0x333333 }, { 47, 767, 0x333333 }, { 48, 767, 0x353535 }, { 49, 767, 0x434343 },
            { 50, 767, 0x444444 }, { 38, 767, 0x333333 }, { 25, 768, 0x444444 }, { 26, 768, 0x333333 }, { 27, 768, 0x622F2F },
            { 28, 768, 0xBD2828 }, { 29, 768, 0xBF2828 }, { 46, 768, 0xBD2828 }, { 47, 768, 0x622F2F }, { 48, 768, 0x333333 },
            { 49, 768, 0x3F3F3F }, { 50, 768, 0x444444 }, { 38, 768, 0xBF2828 }, { 25, 790, 0x444444 }, { 26, 790, 0x434343 },
            { 27, 790, 0x3F3F3F }, { 28, 790, 0x393939 }, { 29, 790, 0x3A3A3A }, { 46, 790, 0x393939 }, { 47, 790, 0x3F3F3F },
            { 48, 790, 0x434343 }, { 49, 790, 0x444444 }, { 50, 790, 0x444444 }, { 38, 790, 0x3B3B3B }, { 25, 791, 0x444444 },
            { 26, 791, 0x444444 }, { 27, 791, 0x444444 }, { 28, 791, 0x444444 }, { 29, 791, 0x444444 }, { 46, 791, 0x444444 },
            { 47, 791, 0x444444 }, { 48, 791, 0x444444 }, { 49, 791, 0x444444 }, { 50, 791, 0x444444 }, { 38, 791, 0x444444 },
            { 25, 769, 0x444444 }, { 25, 770, 0x444444 }, { 25, 771, 0x444444 }, { 25, 772, 0x444444 }, { 25, 785, 0x444444 },
            { 25, 786, 0x444444 }, { 25, 787, 0x444444 }, { 25, 788, 0x444444 }, { 25, 779, 0x444444 }, { 26, 769, 0x333333 },
            { 26, 770, 0x333333 }, { 26, 771, 0x333333 }, { 26, 772, 0x333333 }, { 26, 785, 0x333333 }, { 26, 786, 0x333333 },
            { 26, 787, 0x333333 }, { 26, 788, 0x333333 }, { 26, 779, 0x333333 }, { 48, 769, 0x333333 }, { 48, 770, 0x333333 },
            { 48, 771, 0x333333 }, { 48, 772, 0x333333 }, { 48, 785, 0x333333 }, { 48, 786, 0x333333 }, { 48, 787, 0x333333 },
            { 48, 788, 0x333333 }, { 48, 779, 0x333333 }, { 49, 769, 0x393939 }, { 49, 770, 0x3A3A3A }, { 49, 771, 0x3B3B3B },
            { 49, 772, 0x3B3B3B }, { 49, 785, 0x3B3B3B }, { 49, 786, 0x3A3A3A }, { 49, 787, 0x393939 }, { 49, 788, 0x3F3F3F },
            { 49, 779, 0x3B3B3B }, { 50, 769, 0x444444 }, { 50, 770, 0x444444 }, { 50, 771, 0x444444 }, { 50, 772, 0x444444 },
            { 50, 785, 0x444444 }, { 50, 786, 0x444444 }, { 50, 787, 0x444444 }, { 50, 788, 0x444444 }, { 50, 779, 0x444444 },
        };

        //! the separator 270 x 3 at (17, 236): its top is a whole device row (354)
        const Sample SeparatorWholePhase[] = {
            { 25, 353, 0x444444 }, { 26, 353, 0x444444 }, { 27, 353, 0x444444 }, { 200, 353, 0x444444 },
            { 429, 353, 0x444444 }, { 430, 353, 0x444444 }, { 431, 353, 0x444444 }, { 25, 354, 0x444444 },
            { 26, 354, 0x393A3C }, { 27, 354, 0x393A3C }, { 200, 354, 0x393A3C }, { 429, 354, 0x393A3C },
            { 430, 354, 0x2D2E2F }, { 431, 354, 0x444444 }, { 25, 355, 0x444444 }, { 26, 355, 0x2D2E2F },
            { 27, 355, 0x2D2E2F }, { 200, 355, 0x2D2E2F }, { 429, 355, 0x2D2E2F }, { 430, 355, 0x282829 },
            { 431, 355, 0x444444 }, { 25, 356, 0x444444 }, { 26, 356, 0x222222 }, { 27, 356, 0x222222 },
            { 200, 356, 0x222222 }, { 429, 356, 0x222222 }, { 430, 356, 0x222222 }, { 431, 356, 0x444444 },
            { 25, 357, 0x444444 }, { 26, 357, 0x393A3C }, { 27, 357, 0x393A3C }, { 200, 357, 0x393A3C },
            { 429, 357, 0x393A3C }, { 430, 357, 0x2D2E2F }, { 431, 357, 0x444444 }, { 25, 358, 0x444444 },
            { 26, 358, 0x2D2E2F }, { 27, 358, 0x2D2E2F }, { 200, 358, 0x2D2E2F }, { 429, 358, 0x2D2E2F },
            { 430, 358, 0x282829 }, { 431, 358, 0x444444 }, { 25, 359, 0x444444 }, { 26, 359, 0x444444 },
            { 27, 359, 0x444444 }, { 200, 359, 0x444444 }, { 429, 359, 0x444444 }, { 430, 359, 0x444444 },
            { 431, 359, 0x444444 },
        };

        //! the separator 270 x 3 at (17, 535): its top is half a device row (802.5)
        const Sample SeparatorHalfPhase[] = {
            { 25, 802, 0x444444 }, { 26, 802, 0x444444 }, { 27, 802, 0x444444 }, { 200, 802, 0x444444 },
            { 429, 802, 0x444444 }, { 430, 802, 0x444444 }, { 431, 802, 0x444444 }, { 25, 803, 0x444444 },
            { 26, 803, 0x393A3C }, { 27, 803, 0x393A3C }, { 200, 803, 0x393A3C }, { 429, 803, 0x393A3C },
            { 430, 803, 0x2D2E2F }, { 431, 803, 0x444444 }, { 25, 804, 0x444444 }, { 26, 804, 0x222222 },
            { 27, 804, 0x222222 }, { 200, 804, 0x222222 }, { 429, 804, 0x222222 }, { 430, 804, 0x222222 },
            { 431, 804, 0x444444 }, { 25, 805, 0x444444 }, { 26, 805, 0x2D2E2F }, { 27, 805, 0x2D2E2F },
            { 200, 805, 0x2D2E2F }, { 429, 805, 0x2D2E2F }, { 430, 805, 0x282829 }, { 431, 805, 0x444444 },
            { 25, 806, 0x444444 }, { 26, 806, 0x393A3C }, { 27, 806, 0x393A3C }, { 200, 806, 0x393A3C },
            { 429, 806, 0x393A3C }, { 430, 806, 0x2D2E2F }, { 431, 806, 0x444444 }, { 25, 807, 0x444444 },
            { 26, 807, 0x444444 }, { 27, 807, 0x444444 }, { 200, 807, 0x444444 }, { 429, 807, 0x444444 },
            { 430, 807, 0x444444 }, { 431, 807, 0x444444 },
        };

        //! the swatch 32 x 16 of the second color label of n2 06_color-label, at (494, 584) there; here at (6, 24): the same device
        //! phase (a shift of 488 and of 560 logical pixels is whole device pixels), 732 and 840 less
        const Sample LabelSwatchRing[] = {
            { 8, 35, 0x444444 }, { 9, 35, 0x444444 }, { 10, 35, 0x444444 }, { 32, 35, 0x444444 }, { 55, 35, 0x444444 },
            { 56, 35, 0x444444 }, { 57, 35, 0x444444 }, { 8, 36, 0x444444 }, { 9, 36, 0xFF0000 }, { 10, 36, 0xFF0000 },
            { 32, 36, 0xFF0000 }, { 55, 36, 0xFF0000 }, { 56, 36, 0xFF0000 }, { 57, 36, 0x444444 }, { 8, 37, 0x444444 },
            { 9, 37, 0xFF0000 }, { 10, 37, 0xFF0000 }, { 32, 37, 0xFF0000 }, { 55, 37, 0xFF0000 }, { 56, 37, 0xFF0000 },
            { 57, 37, 0x444444 }, { 8, 59, 0x444444 }, { 9, 59, 0xFF0000 }, { 10, 59, 0xFF0000 }, { 32, 59, 0xFF0000 },
            { 55, 59, 0xFF0000 }, { 56, 59, 0xFF0000 }, { 57, 59, 0x444444 }, { 8, 60, 0x444444 }, { 9, 60, 0x444444 },
            { 10, 60, 0x444444 }, { 32, 60, 0x444444 }, { 55, 60, 0x444444 }, { 56, 60, 0x444444 }, { 57, 60, 0x444444 },
            { 8, 38, 0x444444 }, { 8, 47, 0x444444 }, { 8, 58, 0x444444 }, { 9, 38, 0xFF0000 }, { 9, 47, 0xFF0000 },
            { 9, 58, 0xFF0000 }, { 56, 38, 0xFF0000 }, { 56, 47, 0xFF0000 }, { 56, 58, 0xFF0000 }, { 57, 38, 0x444444 },
            { 57, 47, 0x444444 }, { 57, 58, 0x444444 },
        };
    } // namespace ColorPickerElementTestData

    // The tests and their fixture sit in a namespace of their own: the using-directive below must not leak into the other files
    // of a unity build
    namespace ColorPickerElementTestsInternal
    {
        using namespace ColorPickerElementTestData;

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the Global elements registered
        // ----------------------------------------------------------------------------------

        class ColorPickerElementTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            ColorPickerElementTests()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

                // The fonts and the text colour the application has when StyleManager sets the style (StyleManager.cpp:305-307,
                // Style.cpp buildO3DEBasePalette): the legacy sheet gives labels and buttons the same
                QFont applicationFont(QStringLiteral("Open Sans"));
                applicationFont.setPixelSize(12);
                QApplication::setFont(applicationFont);
                QPalette palette;
                for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                {
                    palette.setColor(role, Qt::white);
                }
                // The page and the base of the flattened GUI (buildO3DEBasePalette): what a widget that does not paint its own background
                // shows. The legacy sheet makes every background transparent, so it shows the page, which is the same color
                palette.setColor(QPalette::Window, QColor(0x44, 0x44, 0x44));
                palette.setColor(QPalette::Base, QColor(0x44, 0x44, 0x44));
                QApplication::setPalette(palette);

                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                TextElement::installClassFonts();

                // Registered once for the process, as the application does from Style::polish(QApplication*)
                ColorPickerElements::registerGlobal();
                PropertyEditorElements::registerGlobal();

                // A palette view reads the clipboard when it is made (PaletteView.cpp:662-666, :682); a platform with no clipboard answers with none
                QGuiApplication::clipboard()->setText(QStringLiteral("color picker element tests"));
            }

            ~ColorPickerElementTests() override
            {
                m_style.reset();
                m_application.reset();
            }

            //! The legacy twin: the sheet on the root, over the application's Fusion. The flat twin: the style on the root and on
            //! everything under it (a widget's style is not its children's).
            static void legacy(QWidget& root, std::initializer_list<const char*> standIns)
            {
                root.setStyleSheet(appSheet(standIns));
            }

            void flat(QWidget& root) const
            {
                root.setStyle(m_style.get());
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(m_style.get());
                }
            }

            static void show(QWidget& root, const QSize& size)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(size);
                root.show();
                QApplication::processEvents();
            }

            //! One tree twice, shown: the build function fills a root with it
            struct Twins
            {
                template<typename Build>
                Twins(const ColorPickerElementTests& tests, Build build, const QSize& size, std::initializer_list<const char*> standIns = {})
                {
                    build(m_legacy);
                    legacy(m_legacy, standIns);
                    show(m_legacy, size);
                    build(m_flat);
                    tests.flat(m_flat);
                    show(m_flat, size);
                }

                template<typename W = QWidget>
                W* legacyChild(const char* name)
                {
                    return m_legacy.findChild<W*>(QString::fromLatin1(name));
                }

                template<typename W = QWidget>
                W* flatChild(const char* name)
                {
                    return m_flat.findChild<W*>(QString::fromLatin1(name));
                }

                QWidget m_legacy;
                QWidget m_flat;
            };

            //! What a window paints, as the backing store of a window of the ratio holds it: the widget and its children into an image
            //! of an explicit device ratio (the page behind them #444444, the color of the picker's)
            static QImage paint(QWidget& root, const QSize& size, qreal ratio)
            {
                QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x44, 0x44, 0x44));
                QPainter painter(&image);
                root.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
                painter.end();
                return image;
            }

            //! The pixels of two images that differ
            static int differing(const QImage& a, const QImage& b)
            {
                if (a.size() != b.size())
                {
                    return a.width() * a.height();
                }
                int count = 0;
                for (int y = 0; y < a.height(); ++y)
                {
                    for (int x = 0; x < a.width(); ++x)
                    {
                        count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                    }
                }
                return count;
            }

            //! Both twins paint alike, at each ratio
            static void expectSamePictures(Twins& twins, const QSize& size, const char* what)
            {
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    const QImage legacyImage = paint(twins.m_legacy, size, ratio);
                    const QImage flatImage = paint(twins.m_flat, size, ratio);
                    EXPECT_EQ(differing(legacyImage, flatImage), 0) << what << " at ratio " << ratio;
                }
            }

            //! The pixels n2 drew, in an image at the ratio of the capture
            static void expectSamples(const QImage& image, const Sample* samples, size_t count, const char* what)
            {
                int wrong = 0;
                for (size_t i = 0; i < count; ++i)
                {
                    const Sample& sample = samples[i];
                    const unsigned actual = image.pixel(sample.m_x, sample.m_y) & 0xFFFFFFu;
                    if (actual != sample.m_rgb && wrong++ < 4)
                    {
                        ADD_FAILURE() << what << ": the pixel (" << sample.m_x << ", " << sample.m_y << ") is "
                                      << QString::number(actual, 16).toStdString() << ", n2's is " << QString::number(sample.m_rgb, 16).toStdString();
                    }
                }
                EXPECT_EQ(wrong, 0) << what;
            }

            template<size_t N>
            static void expectSamples(const QImage& image, const Sample (&samples)[N], const char* what)
            {
                expectSamples(image, samples, N, what);
            }

            static std::string marginsText(const QMargins& margins)
            {
                return QStringLiteral("(%1, %2, %3, %4)")
                    .arg(margins.left()).arg(margins.top()).arg(margins.right()).arg(margins.bottom())
                    .toStdString();
            }

            static std::string sizeText(const QSize& size)
            {
                return QStringLiteral("%1 x %2").arg(size.width()).arg(size.height()).toStdString();
            }

            //! The box Qt derived from the element is the one it derived from the sheet: geometry, contents, limits, frame and hints
            template<typename Twin>
            static void expectSameBox(Twin& twins, std::initializer_list<const char*> names)
            {
                for (const char* name : names)
                {
                    const QWidget* legacyWidget = twins.template legacyChild<QWidget>(name);
                    const QWidget* flatWidget = twins.template flatChild<QWidget>(name);
                    ASSERT_TRUE(legacyWidget != nullptr && flatWidget != nullptr) << name;
                    EXPECT_EQ(flatWidget->geometry(), legacyWidget->geometry()) << name;
                    EXPECT_TRUE(flatWidget->contentsMargins() == legacyWidget->contentsMargins())
                        << name << ": " << marginsText(flatWidget->contentsMargins()) << " against " << marginsText(legacyWidget->contentsMargins());
                    EXPECT_EQ(flatWidget->minimumSize(), legacyWidget->minimumSize()) << name;
                    EXPECT_EQ(flatWidget->maximumSize(), legacyWidget->maximumSize()) << name;
                    EXPECT_EQ(flatWidget->sizeHint(), legacyWidget->sizeHint()) << name;
                    EXPECT_EQ(flatWidget->minimumSizeHint(), legacyWidget->minimumSizeHint()) << name;
                    if (const auto* legacyFrame = qobject_cast<const QFrame*>(legacyWidget))
                    {
                        EXPECT_EQ(qobject_cast<const QFrame*>(flatWidget)->frameWidth(), legacyFrame->frameWidth()) << name;
                    }
                }
            }

            static QScrollArea* makeScrollArea(const char* name, QWidget* parent)
            {
                auto* area = new QScrollArea(parent);
                area->setObjectName(QString::fromLatin1(name));
                area->setWidget(new QWidget());
                // No scroll bars: what is compared is the frame
                area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                area->setFixedSize(200, 100);
                return area;
            }

            static QLabel* makeLabel(QWidget* parent, const char* name, const char* text, const char* tag = nullptr)
            {
                auto* label = new QLabel(QString::fromLatin1(text), parent);
                label->setObjectName(QString::fromLatin1(name));
                if (tag)
                {
                    label->setProperty("class", QString::fromLatin1(tag));
                }
                return label;
            }

            //! The one child of a class a picker has
            template<typename W>
            static W* onlyChild(QWidget& parent, const char* what)
            {
                const QList<W*> children = parent.findChildren<W*>();
                EXPECT_EQ(children.size(), 1) << what;
                return children.isEmpty() ? nullptr : children.first();
            }

            //! The element of a widget in the flattened style, by name ("" for none)
            QString elementOf(QWidget* widget) const
            {
                widget->setStyle(m_style.get());
                widget->ensurePolished();
                const BoxElement* element = BoxElement::forWidget(widget);
                return element ? QString::fromLatin1(element->m_name) : QString();
            }

        protected:
            int m_argc = 1;                                         // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
        };

        // ----------------------------------------------------------------------------------
        // T-CP.1: the scroll area of the picker. ColorPicker.qss:39-43 `border: none`: no frame, the viewport fills the widget (n2
        // 07_color-picker__dialog0: the viewport at the scroll area's origin, the picker 302 wide; the flat one had the frame of the base
        // style, 304).
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, ScrollArea_OfThePickerHasNoFrameAndTakesNoRoom)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const auto build = [](QWidget& root)
            {
                auto* column = new QVBoxLayout(&root);
                auto* picker = new PickerStandIn(PickerClass, "picker", &root);
                column->addWidget(picker);
                auto* inner = new QVBoxLayout(picker);
                inner->addWidget(makeScrollArea("inside", picker));
                column->addWidget(makeScrollArea("outside", &root));
            };
            Twins twins(*this, build, QSize(300, 300), { PickerSelector });

            const auto* legacy = twins.legacyChild<QScrollArea>("inside");
            const auto* flat = twins.flatChild<QScrollArea>("inside");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            EXPECT_EQ(legacy->frameWidth(), 0);
            EXPECT_EQ(flat->frameWidth(), 0);
            EXPECT_TRUE(flat->contentsMargins() == QMargins());
            EXPECT_TRUE(flat->viewport()->geometry() == legacy->viewport()->geometry());
            EXPECT_EQ(flat->viewport()->geometry(), QRect(0, 0, 200, 100));
            EXPECT_EQ(flat->sizeHint(), legacy->sizeHint()) << sizeText(flat->sizeHint()) << " against " << sizeText(legacy->sizeHint());
            EXPECT_EQ(flat->minimumSizeHint(), legacy->minimumSizeHint());

            // A scroll area outside the picker keeps the frame of the base style, whatever it is, the same in both
            const auto* legacyOutside = twins.legacyChild<QScrollArea>("outside");
            const auto* flatOutside = twins.flatChild<QScrollArea>("outside");
            EXPECT_EQ(flatOutside->frameWidth(), legacyOutside->frameWidth());
            EXPECT_TRUE(flatOutside->viewport()->geometry() == legacyOutside->viewport()->geometry());
            EXPECT_GT(flatOutside->frameWidth(), 0);
        }

        // ----------------------------------------------------------------------------------
        // T-CP.2: the labels of the picker. ColorPicker.qss:45-49 `AzQtComponents--ColorPicker QLabel { padding: 0px; margin: 0px }` outranks
        // Text.qss:89's `QLabel` (two type selectors against one), so a label of the picker has no box and no automatic indent (n2 dialog0:
        // the label of the hex field 8 x 17, its margins 0, where the flat label was 14 x 19, margins 0, 1, 0, 1; the title of the quick
        // palette 74 x 17, flat 80 x 21); a tier tag of Text.qss is a class selector and outranks both.
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, Label_InsideThePickerHasNoBoxAndNoIndent)
        {
            const auto build = [](QWidget& root)
            {
                auto* column = new QVBoxLayout(&root);
                column->setContentsMargins(0, 0, 0, 0);
                column->setSpacing(0);
                auto* picker = new PickerStandIn(PickerClass, "picker", &root);
                column->addWidget(picker);
                auto* inner = new QVBoxLayout(picker);
                inner->setContentsMargins(0, 0, 0, 0);
                inner->setSpacing(0);
                inner->addWidget(makeLabel(picker, "plain", "Color space: sRGB"));
                inner->addWidget(makeLabel(picker, "secondary", "Alternate Int", "secondaryText"));
                inner->addWidget(makeLabel(picker, "title", "Quick Palette", "Title"));
                inner->addWidget(makeLabel(picker, "tooltip", "Tooltip text", "Tooltip"));
                // A label with a frame of its own: the rule's box has no margin and no padding, and the frame is the style's (the rule
                // declares no border), so what the label keeps is the width of that frame
                auto* framed = makeLabel(picker, "framed", "Framed label");
                framed->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
                inner->addWidget(framed);
                auto* thick = makeLabel(picker, "thick", "Thick frame");
                thick->setFrameStyle(QFrame::Box | QFrame::Plain);
                thick->setLineWidth(3);
                inner->addWidget(thick);
                // The title of a card header is an ElidingLabel, which measures its hint with a hidden label of its own that
                // ElidingLabel::setObjectName gives the same name (n2: 74 x 17)
                auto* header = new PickerStandIn(CardHeaderClass, "cardHeader", picker);
                auto* headerRow = new QHBoxLayout(header);
                headerRow->setContentsMargins(0, 0, 0, 0);
                auto* name = new ElidingLabel(header);
                name->setObjectName(QStringLiteral("Title"));
                name->setText(QStringLiteral("Quick Palette"));
                headerRow->addWidget(name);
                inner->addWidget(header);
                column->addWidget(makeLabel(&root, "outside", "Outside the picker"));
            };
            Twins twins(*this, build, QSize(320, 240), { PickerSelector, CardHeaderSelector });

            expectSameBox(twins, { "plain", "secondary", "title", "tooltip", "framed", "thick", "Title", "outside" });

            // The untagged labels have no box at all; the tagged ones keep the margin of their tier (Text.qss:64-70, :112-118), and a
            // label outside the picker the 1px above and below of Text.qss:93-94
            for (const char* name : { "plain", "secondary", "Title" })
            {
                const auto* label = twins.flatChild<QLabel>(name);
                EXPECT_TRUE(label->contentsMargins() == QMargins()) << name << ": " << marginsText(label->contentsMargins());
                EXPECT_EQ(label->frameWidth(), 0) << name;
            }
            EXPECT_EQ(twins.flatChild<QLabel>("title")->contentsMargins().top(), 7);
            EXPECT_EQ(twins.flatChild<QLabel>("tooltip")->contentsMargins().top(), 2);
            EXPECT_TRUE(twins.flatChild<QLabel>("outside")->contentsMargins() == QMargins(0, 1, 0, 1));
        }

        // ----------------------------------------------------------------------------------
        // T-CP.2b: every tier of Text.qss, and the other kinds of label, inside the picker. A tier is a class selector, which outranks the two
        // type names of the picker's rule for the margin it sets (Text.qss:55-127: above and below only), so such a label keeps its margin
        // and has the picker's 0 and no padding elsewhere: it is none of the picker's elements (the registry) and its box is the sheet's all
        // the same. A label with a margin or an indent of its own, one that wraps and one that shows a pixmap are the picker's.
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, Label_EveryTextTierInsideThePickerKeepsItsOwnBox)
        {
            static const char* const tiers[] = { "Headline", "Title", "Subtitle", "Menu", "Label", "Paragraph", "Tooltip", "Button", "secondaryText", "primaryText" };
            const auto build = [](QWidget& root)
            {
                auto* column = new QVBoxLayout(&root);
                column->setContentsMargins(0, 0, 0, 0);
                column->setSpacing(0);
                auto* picker = new PickerStandIn(PickerClass, "picker", &root);
                column->addWidget(picker);
                auto* inner = new QVBoxLayout(picker);
                inner->setContentsMargins(0, 0, 0, 0);
                inner->setSpacing(0);
                for (const char* tier : tiers)
                {
                    // the tier alone, and with a second tag (the tags of a label may be several)
                    inner->addWidget(makeLabel(picker, tier, "Some text of the tier", tier));
                    inner->addWidget(makeLabel(picker, (std::string(tier) + "2").c_str(), "Two tags", (std::string(tier) + " secondaryText").c_str()));
                }
                auto* margin = makeLabel(picker, "ownMargin", "Own margin");
                margin->setMargin(5);
                inner->addWidget(margin);
                auto* indent = makeLabel(picker, "ownIndent", "Own indent");
                indent->setIndent(9);
                inner->addWidget(indent);
                auto* wrapped = makeLabel(picker, "wrapped", "A label with a long text that wraps over several lines in a narrow box");
                wrapped->setWordWrap(true);
                inner->addWidget(wrapped);
                auto* pixmapLabel = makeLabel(picker, "pixmap", "");
                QPixmap pixmap(16, 12);
                pixmap.fill(Qt::red);
                pixmapLabel->setPixmap(pixmap);
                inner->addWidget(pixmapLabel);
            };
            Twins twins(*this, build, QSize(260, 600), { PickerSelector });

            std::vector<std::string> names;
            for (const char* tier : tiers)
            {
                names.push_back(tier);
                names.push_back(std::string(tier) + "2");
            }
            for (const char* own : { "ownMargin", "ownIndent", "wrapped", "pixmap" })
            {
                names.push_back(own);
            }
            for (const std::string& name : names)
            {
                expectSameBox(twins, { name.c_str() });
            }
            expectSamePictures(twins, QSize(260, 600), "the labels of every tier");

            // The registry: a label of a Text.qss tier is none of the picker's, the others are
            QWidget root;
            root.setStyle(m_style.get());
            auto* picker = new PickerStandIn(PickerClass, "picker", &root);
            for (const char* tier : { "Headline", "Title", "Subtitle", "Menu", "Label", "Paragraph", "Tooltip", "Button" })
            {
                EXPECT_EQ(elementOf(makeLabel(picker, "tiered", "text", (std::string(tier) + " secondaryText").c_str())), QString()) << tier;
            }
            EXPECT_EQ(elementOf(makeLabel(picker, "colored", "text", "secondaryText primaryText")), QStringLiteral("Global.Box.PickerLabel"));
        }

        // ----------------------------------------------------------------------------------
        // T-CP.3: a label that was polished before the picker was above it. ColorComponentEdit polishes itself and all that is in it (its
        // label, spin box and slider: GetRequiredWidth) while the sliders it sits in have no parent; the legacy sheet matched a widget against
        // its parents again whenever it was reparented (QWidgetPrivate::inheritStyle), the flattened style resolves an element when a widget
        // is polished, and the picker resolves the widgets under it again when it is polished (n2 dialog0: the labels R, G and B 9 x 17,
        // margins 0).
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, Label_PolishedBeforeItsParentsAreResolvedAgainWhenThePickerIsPolished)
        {
            QWidget root;
            root.setStyle(m_style.get());
            auto* picker = new PickerStandIn(PickerClass, "picker", &root);
            picker->setStyle(m_style.get());

            // The sliders of a tab: made with no parent, polished (as ColorComponentEdit does), added to the picker later
            auto* sliders = new QWidget();
            sliders->setStyle(m_style.get());
            auto* label = makeLabel(sliders, "R", "R");
            label->setStyle(m_style.get());
            label->ensurePolished();
            EXPECT_TRUE(BoxElement::forWidget(label) == nullptr);
            EXPECT_TRUE(label->contentsMargins() == QMargins(0, 1, 0, 1)) << marginsText(label->contentsMargins());

            sliders->setParent(picker);
            EXPECT_TRUE(BoxElement::forWidget(label) == nullptr);   // nothing told it: an element is resolved when a widget is polished

            // The style polishes the picker, and the picker's rules reach what is under it
            m_style->polish(picker);
            const BoxElement* element = BoxElement::forWidget(label);
            ASSERT_TRUE(element != nullptr);
            EXPECT_STREQ(element->m_name, "Global.Box.PickerLabel");
            EXPECT_TRUE(label->contentsMargins() == QMargins()) << marginsText(label->contentsMargins());

            // A widget that is no picker does not do this
            QWidget other;
            other.setStyle(m_style.get());
            auto* otherSliders = new QWidget();
            auto* otherLabel = makeLabel(otherSliders, "R", "R");
            otherLabel->setStyle(m_style.get());
            otherLabel->ensurePolished();
            otherSliders->setParent(&other);
            m_style->polish(&other);
            EXPECT_TRUE(BoxElement::forWidget(otherLabel) == nullptr);
        }

        // ----------------------------------------------------------------------------------
        // T-CP.4: the separators between the sections of the picker. ColorPicker.qss:83-90: a QFrame HLine with a 1px line of #393a3c above and
        // below and none at the sides, #222222 between; its box is the two lines (n2: contents margins 0, 1, 0, 1, the flat frame had 1 on every
        // side), and the sheet drew it once, with the styled background. The antialiased lines blend at a fractional device ratio: n2's
        // rows are those of the samples, for a separator whose top is a whole device row and for one whose top is half a row.
        // ----------------------------------------------------------------------------------

        QFrame* makeSeparator(QWidget* parent, const QRect& geometry)
        {
            auto* separator = new QFrame(parent);
            separator->setFrameStyle(QFrame::StyledPanel);
            separator->setFrameShadow(QFrame::Plain);
            separator->setFrameShape(QFrame::HLine);
            separator->setProperty("class", QStringLiteral("HorizontalSeparator"));
            separator->setGeometry(geometry);
            return separator;
        }

        TEST_F(ColorPickerElementTests, Separator_IsTheBoxOfItsTwoLinesAndDrawsAsN2Did)
        {
            const auto build = [](QWidget& root)
            {
                auto* picker = new PickerStandIn(PickerClass, "picker", &root);
                picker->setGeometry(0, 0, 320, 560);
                makeSeparator(picker, QRect(17, 236, 270, 3))->setObjectName(QStringLiteral("whole"));
                makeSeparator(picker, QRect(17, 535, 270, 3))->setObjectName(QStringLiteral("half"));
            };
            Twins twins(*this, build, QSize(320, 560), { PickerSelector });

            for (const char* name : { "whole", "half" })
            {
                const auto* legacy = twins.legacyChild<QFrame>(name);
                const auto* flat = twins.flatChild<QFrame>(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                EXPECT_TRUE(flat->contentsMargins() == QMargins(0, 1, 0, 1)) << name << ": " << marginsText(flat->contentsMargins());
                EXPECT_TRUE(flat->contentsMargins() == legacy->contentsMargins()) << name;
                EXPECT_EQ(flat->sizeHint(), legacy->sizeHint()) << name;
                EXPECT_EQ(flat->frameWidth(), legacy->frameWidth()) << name;
            }
            expectSamePictures(twins, QSize(320, 560), "the separators");

            const QImage flatImage = paint(twins.m_flat, QSize(320, 560), 1.5);
            expectSamples(flatImage, SeparatorWholePhase, "the flat separator whose top is a whole device row, against n2");
            expectSamples(flatImage, SeparatorHalfPhase, "the flat separator whose top is half a device row, against n2");
            const QImage legacyImage = paint(twins.m_legacy, QSize(320, 560), 1.5);
            expectSamples(legacyImage, SeparatorWholePhase, "the legacy twin of the separator whose top is a whole device row, against n2");
            expectSamples(legacyImage, SeparatorHalfPhase, "the legacy twin of the separator whose top is half a device row, against n2");

            // The element is the picker's: a separator outside it is as the frame element makes it, not this box
            QFrame outside;
            outside.setProperty("class", QStringLiteral("HorizontalSeparator"));
            EXPECT_EQ(elementOf(&outside), QString());
        }

        // ----------------------------------------------------------------------------------
        // T-CP.5: the frames of the color grid and the preview. ColorPicker.qss:102-110 `border: 1px solid #333333; border-radius: 2px`, drawn
        // twice by the sheet (the styled background, then the PE_Frame the widget asks for after its contents: n2 shows the half row of the
        // line at 76%, the blend of two 50% draws). The border is the box (n2: contents margins 1, the preview's hint 202 x 22; the flat
        // widgets had 0 and 200 x 20, and the base style's 1px frame over the contents).
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, GridAndPreview_HaveTheBorderBoxOfTheRuleAndDrawItTwice)
        {
            const auto build = [](QWidget& root)
            {
                auto* grid = new ColorGrid(&root);
                grid->setObjectName(QStringLiteral("grid"));
                grid->setGeometry(43, 48, 218, 150);
                auto* preview = new ColorPreview(&root);
                preview->setObjectName(QStringLiteral("preview"));
                preview->setGeometry(43, 206, 218, 22);
                // The colors of a preview start as the memory of its members is (AZ::Color has no default value): the picker's, the color n2 has
                preview->setCurrentColor(AZ::Color(0.0f, 1.0f, 0.0f, 0.5f));
                preview->setSelectedColor(AZ::Color(0.0f, 1.0f, 0.0f, 0.5f));
            };
            Twins twins(*this, build, QSize(320, 240));

            for (const char* name : { "grid", "preview" })
            {
                const auto* legacy = twins.legacyChild<QFrame>(name);
                const auto* flat = twins.flatChild<QFrame>(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                EXPECT_TRUE(flat->contentsMargins() == QMargins(1, 1, 1, 1)) << name << ": " << marginsText(flat->contentsMargins());
                EXPECT_TRUE(flat->contentsMargins() == legacy->contentsMargins()) << name;
                EXPECT_EQ(flat->frameWidth(), 1) << name;
            }
            // (ColorPreview::sizeHint is protected: asked through the widget, as a layout asks it)
            const QWidget* preview = twins.flatChild<ColorPreview>("preview");
            EXPECT_EQ(preview->sizeHint(), QSize(202, 22)) << sizeText(preview->sizeHint());
            EXPECT_EQ(preview->sizeHint(), static_cast<const QWidget*>(twins.legacyChild<ColorPreview>("preview"))->sizeHint());

            expectSamePictures(twins, QSize(320, 240), "the grid and the preview");

            const QImage flatImage = paint(twins.m_flat, QSize(320, 240), 1.5);
            expectSamples(flatImage, GridRing, "the flat color grid, against n2");
            expectSamples(flatImage, PreviewRing, "the flat preview, against n2");
            const QImage legacyImage = paint(twins.m_legacy, QSize(320, 240), 1.5);
            expectSamples(legacyImage, GridRing, "the legacy twin of the color grid, against n2");
            expectSamples(legacyImage, PreviewRing, "the legacy twin of the preview, against n2");
        }

        // ----------------------------------------------------------------------------------
        // T-CP.6: the swatches of a palette and the button that adds one. The view draws its items with a hidden swatch of its own (the
        // delegate: the color in the contents rect, then the PE_Frame of the swatch, once), and the button is a frame that draws its border
        // twice (n2: the swatch 16 x 16, its ring 1px #333333 rounded by 2, the color inside, the button's ring the same with a half
        // row at 76%). The contents of a swatch are 1px in (QFrame::contentsRect): the flat swatches filled the whole square. The view that
        // holds them has no frame of its own (ColorPicker.qss:97-100).
        // ----------------------------------------------------------------------------------

        //! The quick palette of the picker as n2 has it in dialog0: 11 swatches then the add button, 270 x 16 at (17, 511)
        struct PaletteStrip
        {
            PaletteStrip(QWidget& root)
            {
                auto* picker = new PickerStandIn(PickerClass, "picker", &root);
                picker->setGeometry(0, 0, 320, 560);
                m_controller = new Internal::ColorController(&root);
                m_stack = new QUndoStack(&root);
                m_palette = new Palette();
                m_view = new PaletteView(m_palette, m_controller, m_stack, picker);
                m_view->setObjectName(QStringLiteral("view"));
                m_view->addDefaultRGBColors();
                m_view->setGeometry(17, 511, 270, 16);
            }

            ~PaletteStrip()
            {
                delete m_palette;
            }

            Internal::ColorController* m_controller = nullptr;
            QUndoStack* m_stack = nullptr;
            Palette* m_palette = nullptr;
            PaletteView* m_view = nullptr;
        };

        TEST_F(ColorPickerElementTests, PaletteSwatchesAndAddButton_HaveTheBorderBoxOfTheRule)
        {
            std::vector<std::unique_ptr<PaletteStrip>> strips;
            const auto build = [&strips](QWidget& root)
            {
                strips.push_back(std::make_unique<PaletteStrip>(root));
            };
            Twins twins(*this, build, QSize(320, 560), { PickerSelector });

            // The view of the palette has no frame (ColorPicker.qss:97-100; n2: its viewport at its origin and as big), whatever shape it is
            // given: Style::polish takes it off, which the element does not rely on
            auto* legacyView = twins.legacyChild<PaletteView>("view");
            auto* flatView = twins.flatChild<PaletteView>("view");
            ASSERT_TRUE(legacyView != nullptr && flatView != nullptr);
            ASSERT_TRUE(BoxElement::forWidget(flatView) != nullptr);
            EXPECT_STREQ(BoxElement::forWidget(flatView)->m_name, "Global.Box.PickerPaletteView");
            for (PaletteView* view : { legacyView, flatView })
            {
                view->setFrameShape(QFrame::StyledPanel);
                QEvent styleChange(QEvent::StyleChange);
                QCoreApplication::sendEvent(view, &styleChange);
                EXPECT_EQ(view->frameWidth(), 0);
                EXPECT_EQ(view->viewport()->geometry(), QRect(0, 0, 270, 16));
            }

            const auto* legacyButton = twins.legacyChild<QFrame>("addColorButton");
            const auto* flatButton = twins.flatChild<QFrame>("addColorButton");
            ASSERT_TRUE(legacyButton != nullptr && flatButton != nullptr);
            EXPECT_TRUE(flatButton->contentsMargins() == QMargins(1, 1, 1, 1)) << marginsText(flatButton->contentsMargins());
            EXPECT_TRUE(flatButton->contentsMargins() == legacyButton->contentsMargins());
            EXPECT_EQ(flatButton->geometry(), legacyButton->geometry());
            EXPECT_EQ(flatButton->parentWidget()->geometry(), legacyButton->parentWidget()->geometry());
            EXPECT_EQ(flatButton->parentWidget()->mapTo(&twins.m_flat, flatButton->pos()), QPoint(237, 511));

            expectSamePictures(twins, QSize(320, 560), "the palette strip");

            const QImage flatImage = paint(twins.m_flat, QSize(320, 560), 1.5);
            expectSamples(flatImage, SwatchRing, "the first swatch of the flat palette, against n2");
            expectSamples(flatImage, AddSwatchRing, "the add button of the flat palette, against n2");
            const QImage legacyImage = paint(twins.m_legacy, QSize(320, 560), 1.5);
            expectSamples(legacyImage, SwatchRing, "the first swatch of the legacy twin, against n2");
            expectSamples(legacyImage, AddSwatchRing, "the add button of the legacy twin, against n2");
        }

        // ----------------------------------------------------------------------------------
        // T-CP.7: a swatch that is selected. ColorPicker.qss:112-116 `Swatch:selected { border: 2px solid #C8ABFF }` over the rule of the
        // swatch (it keeps the radius of 2px); the color is still laid in the contents rect Qt read when the widget was polished, 1px in.
        // The pseudo-class outranks the `border: 0px` of the swatch of a color label too (ColorLabel.qss:16-25: two type names against
        // 0x10, qcssparser.cpp:1979-1991), so the palette of a picker opened from a color label (the label is among the parents of the
        // dialog) shows its selected swatch with the wide border, while the others of it have none.
        // UNVERIFIED by capture: no n2 screen has a selected swatch; the legacy twin is the proof.
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, Swatch_SelectedHasTheWiderBorderOfTheRule)
        {
            const auto build = [](QWidget& root)
            {
                auto* swatch = new Swatch(&root);
                swatch->setObjectName(QStringLiteral("swatch"));
                swatch->setColor(FromQColor(QColor(0xBF, 0x28, 0x28)));
                swatch->setGeometry(17, 11, 16, 16);
                // The palette view draws its items with a swatch that is hidden: the same, drawn where the item is
                auto* hidden = new Swatch(&root);
                hidden->setObjectName(QStringLiteral("hidden"));
                hidden->setColor(FromQColor(QColor(0x28, 0xBF, 0x28)));
                hidden->setFixedSize(16, 16);
                hidden->hide();
                // A frame that is not a swatch, hidden too: asked for its frame in the state of a selected swatch below
                auto* grid = new ColorGrid(&root);
                grid->setObjectName(QStringLiteral("grid"));
                grid->setFixedSize(30, 30);
                grid->hide();
                // The swatch of a color label (a real one: the sheet's rule names its class), shown right of the swatch above
                auto* label = new ColorLabel(AZ::Colors::Red, &root);
                label->setObjectName(QStringLiteral("label"));
                label->setTextInputVisible(false);
                label->setGeometry(44, 12, 32, 16);
            };
            Twins twins(*this, build, QSize(80, 40));

            const auto draw = [](Swatch* swatch, qreal ratio, QStyle::State state)
            {
                QImage image(QSize(80, 40) * ratio, QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x44, 0x44, 0x44));
                QPainter painter(&image);
                swatch->ensurePolished();
                QStyleOptionFrame option;
                option.initFrom(swatch);
                option.state = state;
                swatch->drawSwatch(&option, &painter, QPoint(17, 11));
                painter.end();
                return image;
            };

            const QStyle::State states[] = {
                QStyle::State(QStyle::State_Enabled),
                QStyle::State(QStyle::State_Enabled | QStyle::State_Selected),
                QStyle::State(QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver),
            };
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                for (const QStyle::State state : states)
                {
                    const QImage legacyImage = draw(twins.legacyChild<Swatch>("hidden"), ratio, state);
                    const QImage flatImage = draw(twins.flatChild<Swatch>("hidden"), ratio, state);
                    EXPECT_EQ(differing(legacyImage, flatImage), 0) << "ratio " << ratio << ", selected " << state.testFlag(QStyle::State_Selected);
                }
            }

            // The swatch of a color label, drawn in the same states: not selected it has no border (its colour to the very edge), selected
            // it has the wide one, as in the twin
            const auto labelSwatch = [&twins](bool flat)
            {
                return flat ? twins.flatChild<ColorLabel>("label")->findChild<Swatch*>() : twins.legacyChild<ColorLabel>("label")->findChild<Swatch*>();
            };
            ASSERT_TRUE(labelSwatch(false) != nullptr && labelSwatch(true) != nullptr);
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                for (const QStyle::State state : states)
                {
                    const QImage legacyImage = draw(labelSwatch(false), ratio, state);
                    const QImage flatImage = draw(labelSwatch(true), ratio, state);
                    EXPECT_EQ(differing(legacyImage, flatImage), 0) << "color label, ratio " << ratio << ", selected " << state.testFlag(QStyle::State_Selected);
                }
            }
            const QImage labelSelected = draw(labelSwatch(true), 1.0, QStyle::State(QStyle::State_Enabled | QStyle::State_Selected));
            EXPECT_EQ(labelSelected.pixelColor(17 + 16, 11).name(), QStringLiteral("#c8abff"));
            EXPECT_EQ(labelSelected.pixelColor(17 + 16, 11 + 1).name(), QStringLiteral("#c8abff"));
            const QImage labelPlain = draw(labelSwatch(true), 1.0, QStyle::State(QStyle::State_Enabled));
            EXPECT_EQ(labelPlain.pixelColor(17 + 16, 11).name(), QStringLiteral("#ff0000"));
            EXPECT_EQ(labelPlain.pixelColor(17, 11 + 8).name(), QStringLiteral("#ff0000"));

            // The rule of ColorPicker.qss:115: the line is #C8ABFF
            const QImage selected = draw(twins.flatChild<Swatch>("hidden"), 1.0, QStyle::State(QStyle::State_Enabled | QStyle::State_Selected));
            EXPECT_EQ(selected.pixelColor(17 + 8, 11).name(), QStringLiteral("#c8abff"));
            EXPECT_EQ(selected.pixelColor(17 + 8, 11 + 1).name(), QStringLiteral("#c8abff"));
            const QImage plain = draw(twins.flatChild<Swatch>("hidden"), 1.0, QStyle::State(QStyle::State_Enabled));
            EXPECT_EQ(plain.pixelColor(17 + 8, 11).name(), QStringLiteral("#333333"));
            EXPECT_EQ(plain.pixelColor(17 + 8, 11 + 1).name(), QStringLiteral("#28bf28"));

            // `Swatch:selected` is the swatch's alone: the frame of the color grid in the same state is the one of its rule
            const auto drawGridFrame = [](QWidget* grid, qreal ratio)
            {
                QImage image(QSize(80, 40) * ratio, QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x44, 0x44, 0x44));
                QPainter painter(&image);
                grid->ensurePolished();
                QStyleOptionFrame option;
                option.initFrom(grid);
                option.rect = QRect(5, 5, 30, 30);
                option.state = QStyle::State(QStyle::State_Enabled | QStyle::State_Selected);
                grid->style()->drawPrimitive(QStyle::PE_Frame, &option, &painter, grid);
                painter.end();
                return image;
            };
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                const QImage legacyGrid = drawGridFrame(twins.legacyChild<ColorGrid>("grid"), ratio);
                const QImage flatGrid = drawGridFrame(twins.flatChild<ColorGrid>("grid"), ratio);
                EXPECT_EQ(differing(legacyGrid, flatGrid), 0) << "ratio " << ratio;
            }
            EXPECT_EQ(drawGridFrame(twins.flatChild<ColorGrid>("grid"), 1.0).pixelColor(5 + 15, 5).name(), QStringLiteral("#333333"));

            // The swatch widget the picker shows is drawn alike
            expectSamePictures(twins, QSize(80, 40), "a swatch");
        }

        // ----------------------------------------------------------------------------------
        // T-CP.8: the swatch of a color label. ColorLabel.qss:16-25 `border: 0px` over ColorPicker.qss:102-110: a flat box of its color (n2
        // 06_color-label: 48 x 24 device pixels of #ff0000, nothing around them). The flat swatch drew the base style's 1px frame over it, and
        // at 1.5 the frame landed on the bottom and right rows the color did not cover (12% of its pixels).
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, ColorLabelSwatch_IsAFlatBoxOfItsColor)
        {
            const auto build = [](QWidget& root)
            {
                // Where n2 has the labels of 06_color-label, less 488 and 560 logical pixels (whole device pixels at 1.5)
                auto* label = new ColorLabel(AZ::Colors::Red, &root);
                label->setObjectName(QStringLiteral("withInput"));
                label->setGeometry(6, 1, 1095, 20);
                auto* noInput = new ColorLabel(AZ::Colors::Red, &root);
                noInput->setObjectName(QStringLiteral("noInput"));
                noInput->setTextInputVisible(false);
                noInput->setGeometry(6, 24, 1095, 16);
                // A swatch with a frame of its own (a thick one, so that the base style's width and the rule's cannot be mistaken): the rule's
                // `border: 0px` is the border of the swatch, not the style's frame, and the box of the rule has no room for a frame
                auto* framed = new ColorLabel(AZ::Colors::Red, &root);
                framed->setObjectName(QStringLiteral("framed"));
                framed->setTextInputVisible(false);
                framed->setGeometry(6, 44, 1095, 16);
                framed->findChild<Swatch*>()->setFrameStyle(QFrame::Box | QFrame::Plain);
                framed->findChild<Swatch*>()->setLineWidth(3);
            };
            Twins twins(*this, build, QSize(160, 60));

            for (const char* name : { "withInput", "noInput", "framed" })
            {
                const auto* legacy = twins.legacyChild<ColorLabel>(name)->findChild<Swatch*>();
                const auto* flat = twins.flatChild<ColorLabel>(name)->findChild<Swatch*>();
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                EXPECT_TRUE(flat->contentsMargins() == QMargins()) << name << ": " << marginsText(flat->contentsMargins());
                EXPECT_TRUE(flat->contentsMargins() == legacy->contentsMargins()) << name;
                EXPECT_EQ(flat->size(), QSize(32, 16)) << name;
                EXPECT_EQ(flat->size(), legacy->size()) << name;
                EXPECT_EQ(flat->frameWidth(), 0) << name;
            }
            EXPECT_STREQ(BoxElement::forWidget(twins.flatChild<ColorLabel>("noInput")->findChild<Swatch*>())->m_name, "Global.Box.PickerLabelSwatch");

            // The text field of the label is the field's (LineEdit, BrowseEdit...): what is compared is the swatch
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                const QImage legacyImage = paint(twins.m_legacy, QSize(160, 60), ratio);
                const QImage flatImage = paint(twins.m_flat, QSize(160, 60), ratio);
                for (const QRect& swatch : { QRect(6, 3, 32, 16), QRect(6, 24, 32, 16), QRect(6, 44, 32, 16) })
                {
                    const QRect device = QRect(swatch.topLeft() * ratio, swatch.size() * ratio).adjusted(-2, -2, 2, 2);
                    EXPECT_EQ(differing(legacyImage.copy(device), flatImage.copy(device)), 0) << "ratio " << ratio;
                }
            }
            const QImage flatImage = paint(twins.m_flat, QSize(160, 60), 1.5);
            expectSamples(flatImage, LabelSwatchRing, "the flat swatch of a color label, against n2");
            const QImage legacyImage = paint(twins.m_legacy, QSize(160, 60), 1.5);
            expectSamples(legacyImage, LabelSwatchRing, "the legacy twin of the swatch of a color label, against n2");
        }

        // ----------------------------------------------------------------------------------
        // T-CP.9: the tool tip of a gradient slider. ColorPicker.qss:118-135 sets qproperty-toolTipOffsetX and Y: 24 above the pointer for a
        // horizontal slider, 40 above it for one in the HSL or HSV tab, 8 right and 24 above for a vertical one. The sheet set them whenever it
        // polished the slider; no capture shows a tool tip (UNVERIFIED), the legacy twin is the proof.
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, GradientSlider_ToolTipOffsetsAreThoseOfTheRules)
        {
            const auto addSlider = [](QWidget* parent, const char* name, Qt::Orientation orientation)
            {
                auto* slider = new GradientSlider(orientation, parent);
                slider->setObjectName(QString::fromLatin1(name));
                // The classes Slider::polish gives a slider (Slider.cpp:504-509): the legacy twin has not the style that adds them
                slider->setProperty("class", orientation == Qt::Horizontal ? QStringLiteral("GradientSlider HorizontalSlider")
                                                                           : QStringLiteral("GradientSlider VerticalSlider"));
                return slider;
            };
            const auto build = [&addSlider](QWidget& root)
            {
                auto* column = new QVBoxLayout(&root);
                addSlider(&root, "horizontal", Qt::Horizontal);
                addSlider(&root, "vertical", Qt::Vertical);
                auto* hsl = new PickerStandIn(HslSlidersClass, "hsl", &root);
                column->addWidget(hsl);
                addSlider(hsl, "inHsl", Qt::Horizontal);
                auto* hsv = new PickerStandIn(HsvSlidersClass, "hsv", &root);
                column->addWidget(hsv);
                addSlider(hsv, "inHsv", Qt::Horizontal);
                auto* nested = new QWidget(hsv);
                addSlider(nested, "inHsvNested", Qt::Horizontal);
            };
            Twins twins(*this, build, QSize(200, 300), { HslSlidersSelector, HsvSlidersSelector });

            const struct
            {
                const char* m_name;
                QPoint m_offset;
            } expected[] = {
                { "horizontal", QPoint(0, -24) },       // ColorPicker.qss:120-121
                { "vertical", QPoint(8, -24) },         // :133-134
                { "inHsl", QPoint(0, -40) },            // :127-128
                { "inHsv", QPoint(0, -40) },
                { "inHsvNested", QPoint(0, -40) },      // any depth: a descendant selector
            };
            for (const auto& row : expected)
            {
                const auto* legacy = twins.legacyChild<GradientSlider>(row.m_name);
                const auto* flat = twins.flatChild<GradientSlider>(row.m_name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << row.m_name;
                EXPECT_EQ(legacy->toolTipOffset(), row.m_offset) << row.m_name;
                EXPECT_EQ(flat->toolTipOffset(), row.m_offset) << row.m_name << ": " << flat->toolTipOffsetX() << ", " << flat->toolTipOffsetY();
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CP.10: the registry. What each element covers is what its rule's selector covered: the widgets of the picker and the color label, and
        // no others; of the rules that reach one widget, the more specific one is the element.
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, Registry_EachElementCoversTheWidgetsOfItsSelector)
        {
            QWidget root;
            root.setStyle(m_style.get());
            auto* picker = new PickerStandIn(PickerClass, "picker", &root);
            auto* header = new PickerStandIn(CardHeaderClass, "cardHeader", picker);
            auto* colorLabel = new ColorLabel(AZ::Colors::Red, &root);

            // the scroll area, the labels and the separator of the picker
            EXPECT_EQ(elementOf(makeScrollArea("area", picker)), QStringLiteral("Global.Box.PickerScrollArea"));
            EXPECT_EQ(elementOf(makeScrollArea("area", &root)), QString());
            EXPECT_EQ(elementOf(makeLabel(picker, "text", "text")), QStringLiteral("Global.Box.PickerLabel"));
            EXPECT_EQ(elementOf(makeLabel(&root, "text", "text")), QString());
            EXPECT_EQ(elementOf(makeLabel(header, "Name", "text")), QStringLiteral("Global.Box.PickerLabel"));
            EXPECT_EQ(elementOf(makeLabel(picker, "text", "text", "secondaryText")), QStringLiteral("Global.Box.PickerLabel"));
            EXPECT_EQ(elementOf(makeSeparator(picker, QRect(0, 0, 10, 3))), QStringLiteral("Global.Box.PickerSeparator"));
            EXPECT_EQ(elementOf(makeSeparator(&root, QRect(0, 0, 10, 3))), QString());

            // a label whose box another rule gives: a tier of Text.qss (a class selector outranks the two type selectors of the picker's), or
            // the icons of a card header (their ids do, and PropertyEditorElements has them)
            for (const char* tier : { "Headline", "Title", "Subtitle", "Menu", "Label", "Paragraph", "Tooltip", "Button" })
            {
                EXPECT_EQ(elementOf(makeLabel(picker, "text", "text", tier)), QString()) << tier;
            }
            EXPECT_EQ(elementOf(makeLabel(header, "Icon", "text")), QStringLiteral("Global.Box.CardHeaderIconInPicker"));
            EXPECT_EQ(elementOf(makeLabel(header, "WarningIcon", "text")), QStringLiteral("Global.Box.CardHeaderWarningIconInPicker"));

            // the frames: the grid, the preview and the swatch anywhere (ColorPicker.qss:102-105 names no ancestor), the swatch of a color
            // label as the later, more specific rule of ColorLabel.qss:16-25 has it
            EXPECT_EQ(elementOf(new ColorGrid(&root)), QStringLiteral("Global.Box.PickerFrame"));
            EXPECT_EQ(elementOf(new ColorPreview(&root)), QStringLiteral("Global.Box.PickerFrame"));
            EXPECT_EQ(elementOf(new Swatch(&root)), QStringLiteral("Global.Box.PickerFrame"));
            EXPECT_EQ(elementOf(colorLabel->findChild<Swatch*>()), QStringLiteral("Global.Box.PickerLabelSwatch"));

            // the tool tip of a slider: the classes the rules name, which Slider::polish gives a slider; the HSL and HSV tabs' sliders have the
            // more specific rule
            EXPECT_EQ(elementOf(new GradientSlider(Qt::Horizontal, &root)), QStringLiteral("Global.Box.PickerHorizontalSliderToolTip"));
            EXPECT_EQ(elementOf(new GradientSlider(Qt::Vertical, &root)), QStringLiteral("Global.Box.PickerVerticalSliderToolTip"));
            auto* hsl = new PickerStandIn(HslSlidersClass, "hsl", &root);
            EXPECT_EQ(elementOf(new GradientSlider(Qt::Horizontal, hsl)), QStringLiteral("Global.Box.PickerTallHorizontalSliderToolTip"));
            auto* hsv = new PickerStandIn(HsvSlidersClass, "hsv", &root);
            EXPECT_EQ(elementOf(new GradientSlider(Qt::Horizontal, hsv)), QStringLiteral("Global.Box.PickerTallHorizontalSliderToolTip"));
        }

        // ----------------------------------------------------------------------------------
        // T-CP.11: the picker as a whole, with the numbers n2 recorded for it (07_color-picker__dialog0, in logical pixels; none depends on a
        // font): the scroll area without a frame, the frames of the grid, the preview and the add button 1px thick, the separators with their
        // two lines, the labels without a box, the labels of the component edits too (they are polished before the picker is above them).
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, ColorPicker_HasTheBoxesN2Recorded)
        {
            QTemporaryDir settingsDirectory;
            ASSERT_TRUE(settingsDirectory.isValid());
            const QString organization = QCoreApplication::organizationName();
            const QString application = QCoreApplication::applicationName();
            const QSettings::Format format = QSettings::defaultFormat();
            QCoreApplication::setOrganizationName(QStringLiteral("o3de-color-picker-element-tests"));
            QCoreApplication::setApplicationName(QStringLiteral("tests"));
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
            const auto restore = [&]()
            {
                QCoreApplication::setOrganizationName(organization);
                QCoreApplication::setApplicationName(application);
                QSettings::setDefaultFormat(format);
            };

            // As the application has the style: on the application, so that what is polished early is polished by it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
            {
                ColorPicker picker(ColorPicker::Configuration::RGBA, QStringLiteral("element tests"));
                picker.setCurrentColor(AZ::Color(0.0f, 1.0f, 0.0f, 0.5f));
                picker.parentWidget()->setAttribute(Qt::WA_DontShowOnScreen);
                picker.show();
                for (int i = 0; i < 5; ++i)
                {
                    QApplication::processEvents();
                }

                // ColorPicker.qss:39-43: the scroll area has no frame, 302 x 531 with margins 0 (n2), the viewport at its origin
                auto* area = onlyChild<QScrollArea>(picker, "scroll area");
                ASSERT_TRUE(area != nullptr);
                EXPECT_EQ(area->frameWidth(), 0);
                EXPECT_EQ(area->viewport()->pos(), QPoint(0, 0));

                // ColorPicker.qss:97-100: the views of the palettes have no frame (n2: the viewport at the view's origin)
                int views = 0;
                for (const auto* view : picker.findChildren<PaletteView*>())
                {
                    EXPECT_EQ(view->frameWidth(), 0);
                    EXPECT_EQ(view->viewport()->pos(), QPoint(0, 0));
                    ++views;
                }
                EXPECT_GE(views, 1);

                // ColorPicker.qss:102-110: 1px
                auto* grid = onlyChild<ColorGrid>(picker, "grid");
                auto* preview = onlyChild<ColorPreview>(picker, "preview");
                ASSERT_TRUE(grid != nullptr && preview != nullptr);
                EXPECT_TRUE(grid->contentsMargins() == QMargins(1, 1, 1, 1));
                EXPECT_TRUE(preview->contentsMargins() == QMargins(1, 1, 1, 1));
                EXPECT_EQ(static_cast<const QWidget*>(preview)->sizeHint(), QSize(202, 22));
                const auto* addButton = picker.findChild<QFrame*>(QStringLiteral("addColorButton"));
                ASSERT_TRUE(addButton != nullptr);
                EXPECT_TRUE(addButton->contentsMargins() == QMargins(1, 1, 1, 1));

                // ColorPicker.qss:83-90: the two lines (n2: four in the container, one of them hidden, and the one above the buttons)
                int separators = 0;
                for (const auto* frame : picker.findChildren<QFrame*>())
                {
                    if (frame->frameShape() == QFrame::HLine && frame->testAttribute(Qt::WA_WState_Polished))
                    {
                        EXPECT_TRUE(frame->contentsMargins() == QMargins(0, 1, 0, 1)) << marginsText(frame->contentsMargins());
                        ++separators;
                    }
                }
                EXPECT_GE(separators, 4);

                // ColorPicker.qss:45-49: no box on any label, those of the component edits among them
                int labels = 0;
                for (const auto* label : picker.findChildren<QLabel*>())
                {
                    if (label->objectName() == QLatin1String("Icon") || label->objectName() == QLatin1String("WarningIcon"))
                    {
                        continue;       // the box of an icon is its id's (Card.qss:214-227)
                    }
                    EXPECT_TRUE(label->contentsMargins() == QMargins())
                        << label->text().toStdString() << ": " << marginsText(label->contentsMargins());
                    EXPECT_EQ(label->frameWidth(), 0) << label->text().toStdString();
                    ++labels;
                }
                EXPECT_GE(labels, 12);          // R G B H S L V, '#', R G B A, the title, the color space (and the hidden ones)

                // ColorPicker.qss:118-135: the tool tip offsets of the sliders (the HSL and HSV tabs are hidden, and polished all the same): the
                // three sliders of each of those tabs have the taller offset, the others the offset of their orientation
                int tall = 0;
                for (const auto* slider : picker.findChildren<GradientSlider*>())
                {
                    const bool inHslOrHsv = TextElement::withinClass(slider, "AzQtComponents::HSLSliders")
                        || TextElement::withinClass(slider, "AzQtComponents::HSVSliders");
                    const QPoint expected = slider->orientation() == Qt::Vertical ? QPoint(8, -24) : (inHslOrHsv ? QPoint(0, -40) : QPoint(0, -24));
                    EXPECT_EQ(slider->toolTipOffset(), expected) << slider->toolTipOffsetX() << ", " << slider->toolTipOffsetY();
                    tall += inHslOrHsv ? 1 : 0;
                }
                EXPECT_EQ(tall, 6);
                picker.hide();
            }
            restore();
        }

        // ----------------------------------------------------------------------------------
        // T-CP.12: the title of a modified palette. ColorPicker.qss:61-64 `AzQtComponents--PaletteCard[modified="true"] AzQtComponents--CardHeader
        // #Title { color: #ffa500 }`: the sheet matched it again whenever the property changed (PaletteCard::setModified recomputes the sheet of
        // the header), and the flattened GUI polishes the header again when the card says it changed. UNVERIFIED by capture: no n2 screen has
        // a palette card; the legacy twin is the proof.
        // ----------------------------------------------------------------------------------

        //! What a palette card needs to outlive it
        struct CardParts
        {
            Internal::ColorController m_controller;
            QUndoStack m_stack;
        };

        TEST_F(ColorPickerElementTests, PaletteCardTitle_IsOrangeWhileThePaletteIsModified)
        {
            std::vector<std::unique_ptr<CardParts>> parts;
            const auto build = [&parts](QWidget& root)
            {
                parts.push_back(std::make_unique<CardParts>());
                auto* layout = new QVBoxLayout(&root);
                layout->setContentsMargins(0, 0, 0, 0);
                auto* card = new PaletteCard(QSharedPointer<Palette>::create(), &parts.back()->m_controller, &parts.back()->m_stack, &root);
                card->setObjectName(QStringLiteral("card"));
                card->setTitle(QStringLiteral("Library"));
                layout->addWidget(card);
            };
            Twins twins(*this, build, QSize(300, 100));

            const auto titleColor = [](Twins& twin, bool flat)
            {
                auto* card = (flat ? twin.flatChild<PaletteCard>("card") : twin.legacyChild<PaletteCard>("card"));
                QLabel* title = nullptr;
                for (QLabel* label : card->findChildren<QLabel*>(QStringLiteral("Title")))
                {
                    title = label->isVisibleTo(card) ? label : title;
                }
                EXPECT_TRUE(title != nullptr);
                return title ? title->palette().color(QPalette::Active, QPalette::WindowText) : QColor();
            };
            const auto titleWeight = [](Twins& twin, bool flat)
            {
                auto* card = (flat ? twin.flatChild<PaletteCard>("card") : twin.legacyChild<PaletteCard>("card"));
                return card->findChild<ElidingLabel*>(QStringLiteral("Title"))->font().weight();
            };

            // Not modified: both are white (the application's text color)
            EXPECT_EQ(titleColor(twins, false).name(), QStringLiteral("#ffffff"));
            EXPECT_EQ(titleColor(twins, true).name(), QStringLiteral("#ffffff"));

            twins.legacyChild<PaletteCard>("card")->setModified(true);
            twins.flatChild<PaletteCard>("card")->setModified(true);
            QApplication::processEvents();
            EXPECT_EQ(titleColor(twins, false).name(), QStringLiteral("#ffa500"));
            EXPECT_EQ(titleColor(twins, true).name(), QStringLiteral("#ffa500"));
            EXPECT_EQ(titleWeight(twins, true), titleWeight(twins, false));

            twins.legacyChild<PaletteCard>("card")->setModified(false);
            twins.flatChild<PaletteCard>("card")->setModified(false);
            QApplication::processEvents();
            EXPECT_EQ(titleColor(twins, false).name(), QStringLiteral("#ffffff"));
            EXPECT_EQ(titleColor(twins, true).name(), QStringLiteral("#ffffff"));
        }

        // ----------------------------------------------------------------------------------
        // T-CP.13: the door is the picker's alone. The frame of a widget that has none of the elements (a QFrame the color picker has nothing to
        // do with, a scroll area outside it) is still the base style's: PE_Frame through the flattened style is what Fusion draws.
        // ----------------------------------------------------------------------------------

        TEST_F(ColorPickerElementTests, Frame_OfAWidgetWithoutAnElementIsStillTheBaseStyles)
        {
            QWidget root;
            auto* frame = new QFrame(&root);
            frame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
            frame->setGeometry(2, 2, 40, 20);
            root.setStyle(m_style.get());
            frame->setStyle(m_style.get());
            frame->ensurePolished();
            ASSERT_TRUE(BoxElement::forWidget(frame) == nullptr);

            std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                const auto draw = [&](const QStyle* style)
                {
                    QImage image(QSize(48, 28) * ratio, QImage::Format_ARGB32_Premultiplied);
                    image.setDevicePixelRatio(ratio);
                    image.fill(QColor(0x44, 0x44, 0x44));
                    QPainter painter(&image);
                    QStyleOptionFrame option;
                    option.initFrom(frame);
                    option.rect = QRect(2, 2, 40, 20);
                    style->drawPrimitive(QStyle::PE_Frame, &option, &painter, frame);
                    painter.end();
                    return image;
                };
                EXPECT_EQ(differing(draw(m_style.get()), draw(fusion.get())), 0) << "ratio " << ratio;
            }

            QStyleOptionFrame option;
            option.initFrom(frame);
            QImage image(QSize(48, 28), QImage::Format_ARGB32_Premultiplied);
            QPainter painter(&image);
            EXPECT_FALSE(ColorPickerElements::drawFrame(&option, &painter, frame));
            EXPECT_FALSE(ColorPickerElements::drawFrame(&option, &painter, nullptr));
        }
    } // namespace ColorPickerElementTestsInternal
} // namespace AzQtComponents
