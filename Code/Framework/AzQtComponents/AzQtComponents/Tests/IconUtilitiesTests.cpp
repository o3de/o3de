/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Utilities/IconUtilities.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QLineEdit>
#include <QStyleFactory>
#include <QTemporaryDir>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // CachedIcon: one icon per image file (and size, and disabled image), shared, painting as a new icon of the same files does
    // ----------------------------------------------------------------------------------

    class IconUtilitiesTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        IconUtilitiesTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
        }

        ~IconUtilitiesTests() override
        {
            m_application.reset();
        }

    protected:
        //! A 16 x 16 SVG image of a square of the colour, in the test's own directory: its path
        QString WriteSvg(const QString& name, const char* fill) const
        {
            const QString path = m_directory.filePath(name);
            QFile file(path);
            EXPECT_TRUE(file.open(QIODevice::WriteOnly));
            file.write(QByteArray("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\">"
                                  "<rect x=\"3\" y=\"2\" width=\"10\" height=\"12\" fill=\"") + fill + "\"/></svg>");
            return path;
        }

        //! The icon as QIcon(filePath) builds it: a new one
        static QIcon NewIcon(const QString& filePath, const QSize& size = QSize(), const QString& disabledFilePath = QString())
        {
            QIcon icon;
            icon.addFile(filePath, size, QIcon::Normal);
            if (!disabledFilePath.isEmpty())
            {
                icon.addFile(disabledFilePath, size, QIcon::Disabled);
            }
            return icon;
        }

        static QImage Paint(const QIcon& icon, const QSize& size, qreal devicePixelRatio, QIcon::Mode mode)
        {
            return icon.pixmap(size, devicePixelRatio, mode).toImage();
        }

        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        QTemporaryDir m_directory;
    };

    TEST_F(IconUtilitiesTests, AFileGivesOneSharedIcon)
    {
        const QString path = WriteSvg(QStringLiteral("square.svg"), "#ff0000");
        const QIcon first = CachedIcon(path);
        ASSERT_FALSE(first.isNull());

        // the same icon again, where building one gives another
        EXPECT_EQ(CachedIcon(path).cacheKey(), first.cacheKey());
        EXPECT_NE(NewIcon(path).cacheKey(), first.cacheKey());
    }

    TEST_F(IconUtilitiesTests, TheSharedIconPaintsAsANewIconOfTheFile)
    {
        const QString path = WriteSvg(QStringLiteral("square.svg"), "#00ff00");
        CachedIcon(path).pixmap(QSize(16, 16));     // painted once before, as a shared icon is

        for (const QSize size : { QSize(16, 16), QSize(12, 12), QSize(24, 20) })
        {
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                for (const QIcon::Mode mode : { QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected })
                {
                    EXPECT_EQ(Paint(CachedIcon(path), size, ratio, mode), Paint(NewIcon(path), size, ratio, mode))
                        << size.width() << "x" << size.height() << " ratio " << ratio << " mode " << mode;
                }
            }
        }
    }

    TEST_F(IconUtilitiesTests, ASizeOrADisabledImageMakesAnotherIcon)
    {
        const QString path = WriteSvg(QStringLiteral("square.svg"), "#0000ff");
        const QString disabledPath = WriteSvg(QStringLiteral("disabled.svg"), "#808080");
        const QSize size(16, 16);

        const QIcon plain = CachedIcon(path);
        const QIcon sized = CachedIcon(path, size);
        const QIcon withDisabled = CachedIcon(path, size, disabledPath);
        EXPECT_NE(sized.cacheKey(), plain.cacheKey());
        EXPECT_NE(withDisabled.cacheKey(), sized.cacheKey());
        EXPECT_EQ(CachedIcon(path, size).cacheKey(), sized.cacheKey());
        EXPECT_EQ(CachedIcon(path, size, disabledPath).cacheKey(), withDisabled.cacheKey());

        // each paints as the icon built from the same files
        for (const QIcon::Mode mode : { QIcon::Normal, QIcon::Disabled })
        {
            EXPECT_EQ(Paint(sized, size, 1.0, mode), Paint(NewIcon(path, size), size, 1.0, mode));
            EXPECT_EQ(Paint(withDisabled, size, 1.0, mode), Paint(NewIcon(path, size, disabledPath), size, 1.0, mode));
        }
        EXPECT_NE(Paint(withDisabled, size, 1.0, QIcon::Disabled), Paint(sized, size, 1.0, QIcon::Disabled));
    }

    TEST_F(IconUtilitiesTests, AFileThatDoesNotExistYetIsLookedForAgain)
    {
        // a component's icon the Asset Processor has not written yet shows once it has
        const QString path = m_directory.filePath(QStringLiteral("later.svg"));
        EXPECT_TRUE(CachedIcon(path).isNull());

        WriteSvg(QStringLiteral("later.svg"), "#ff00ff");
        EXPECT_FALSE(CachedIcon(path).isNull());
    }

    // ----------------------------------------------------------------------------------
    // The icons the style and the widgets build each time they are asked for one are shared
    // ----------------------------------------------------------------------------------

    TEST_F(IconUtilitiesTests, BrowseEditsShareTheirButtonsIcon)
    {
        BrowseEdit first;
        BrowseEdit second;
        ASSERT_FALSE(first.attachedButtonIcon().isNull());
        EXPECT_EQ(first.attachedButtonIcon().cacheKey(), second.attachedButtonIcon().cacheKey());
    }

    TEST_F(IconUtilitiesTests, TheStylesLineEditClearIconIsShared)
    {
        // as StyleManager::initialize builds the style (StyleManager.cpp:244-246); the application owns it
        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        QLineEdit lineEdit;
        const QIcon first = QApplication::style()->standardIcon(QStyle::SP_LineEditClearButton, nullptr, &lineEdit);
        ASSERT_FALSE(first.isNull());
        EXPECT_EQ(QApplication::style()->standardIcon(QStyle::SP_LineEditClearButton, nullptr, &lineEdit).cacheKey(), first.cacheKey());
    }
} // namespace AzQtComponents
