/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <AzQtComponents/Components/Widgets/VectorInput.h>

#include <QLabel>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // C187: the text a caller gives a vector label (VectorInput::setLabelStyle). The legacy GUI sets it as the label's own sheet, a rule
    // of declarations that outranks every rule of the application's (`font: bold; color: rgb(184,51,51);` is the Gallery's,
    // SpinBoxPage.cpp:196-198). It is a legacy-only sheet (StyleManager::setLegacyStyleSheet): the flattened GUI sets no sheet and reads
    // none. The Gallery's own look in the flattened GUI is three rows of SpinBoxPage.cpp (no twin here: the Gallery's sources are not
    // linked into this target; the capture A/B of the Gallery covers it).
    // ----------------------------------------------------------------------------------

    namespace VectorLabelTextFlatSheetTestsInternal
    {
        using namespace SheetTwins;

        class VectorLabelTextFlatSheetTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // SpinBoxPage.cpp:196-198 (the Gallery's styles of the vector, one per coordinate)
        const char* const GalleryBold = "font: bold; color: rgb(184,51,51);";
        const char* const GalleryGreen = "font: bold; color: rgb(48,208,120);";
        const char* const GalleryBlue = "font: bold; color: rgb(66,133,244);";

        //! The label of the element at `index` of a vector the test made, at the rung of the scope it runs in
        QLabel* labelOf(VectorInput& vector, int index)
        {
            return vector.getElements()[index]->getLabelWidget();
        }

        // ----------------------------------------------------------------------------------
        // The sites: the sheet of the legacy GUI is the text as it was, the flattened GUI has none
        // ----------------------------------------------------------------------------------

        TEST_F(VectorLabelTextFlatSheetTests, SetLabelStyle_SetsTheCallersTextAsBeforeInTheLegacyGuiAndNoSheetInTheFlatOne)
        {
            const QStringList texts = { QString::fromLatin1(GalleryBold), QStringLiteral("QLabel { color: red; }"), QString() };
            for (const QString& text : texts)
            {
                {
                    RungScope rung(Rung::Qss);
                    QWidget host;
                    VectorInput vector(&host, 3);
                    vector.setLabelStyle(1, text);
                    EXPECT_EQ(labelOf(vector, 1)->styleSheet(), text);
                    EXPECT_TRUE(labelOf(vector, 0)->styleSheet().isEmpty());
                }
                for (const Rung flatRung : { Rung::Flat, Rung::Noop, Rung::Passthrough })
                {
                    RungScope rung(flatRung);
                    QWidget host;
                    VectorInput vector(&host, 3);
                    vector.setLabelStyle(1, text);
                    EXPECT_TRUE(labelOf(vector, 1)->styleSheet().isEmpty()) << GuiPerfFloor::rungName(flatRung);
                    EXPECT_FALSE(labelOf(vector, 1)->style()->inherits("QStyleSheetStyle")) << GuiPerfFloor::rungName(flatRung);
                }
            }
        }

        TEST_F(VectorLabelTextFlatSheetTests, SetLabelStyle_TheCensusListsTheLabelsInTheLegacyGuiAndNoneInTheFlatOne)
        {
            const auto build = []()
            {
                QWidget host;
                host.resize(300, 100);
                auto* vector = new VectorInput(&host, 3);
                vector->setLabelStyle(0, QString::fromLatin1(GalleryBold));
                vector->setLabelStyle(1, QString::fromLatin1(GalleryGreen));
                vector->setLabelStyle(2, QString::fromLatin1(GalleryBlue));
                host.setAttribute(Qt::WA_DontShowOnScreen);
                host.show();
                host.ensurePolished();
            };

            const QStringList legacy = SiteTwins::sheetLinesAt(Rung::Qss, build);
            ASSERT_EQ(legacy.size(), 3) << legacy.join("\n").toStdString();
            for (const QString& line : legacy)
            {
                EXPECT_TRUE(SiteTwins::namesCarrier(line, QStringLiteral("QLabel#SpinboxDragLabel"))) << line.toStdString();
            }

            const QStringList flat = SiteTwins::sheetLinesAt(Rung::Flat, build);
            EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
        }
    } // namespace VectorLabelTextFlatSheetTestsInternal
} // namespace AzQtComponents
