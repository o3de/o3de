/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QColor>
#include <QString>

namespace EditorFlatValues
{
    //! The paint values that the legacy GUI's inline sheets and the elements of the flattened GUI (EditorFlatSheet.cpp) both read, so
    //! that each one has a single source: the flattened system owns it, and the text of a sheet is written from it. A named namespace,
    //! not an anonymous one: a unity build joins the files of a target into one translation unit, where anonymous namespaces merge.

    // ------------------------------------------------------------------------------
    // The asset browser window (AzAssetBrowserWindow.ui, the form before the flattening)
    // ------------------------------------------------------------------------------

    // The line of the dividers by the Create button (framePreCreate, framePostCreate), a sheet on each:
    // `QFrame { color : rgb(34, 34, 34);}` (AzAssetBrowserWindow.ui:132, :206). TOKENIZE: AssetBrowserCreateDividerLine
    inline constexpr QColor AssetBrowserCreateDividerLine(34, 34, 34);

    // The fill of the thumbnails (m_thumbnailView), a sheet on the view: `{ background-color: #333333; }`
    // (AzAssetBrowserWindow.ui:567). TOKENIZE: AssetBrowserThumbnailsFill
    inline constexpr QColor AssetBrowserThumbnailsFill(0x33, 0x33, 0x33);

    // ------------------------------------------------------------------------------
    // The asset importer's root display (ImporterRootDisplay.ui, the form before the flattening)
    // ------------------------------------------------------------------------------

    // The fill of the python builder frame (m_pythonBuilderLayout), a sheet on the frame: `background-color: #333333`
    // (ImporterRootDisplay.ui:79). TOKENIZE: AssetImporterPythonBuilderFill
    inline constexpr QColor AssetImporterPythonBuilderFill(0x33, 0x33, 0x33);

    // ------------------------------------------------------------------------------
    // The startup logo dialog (StartupLogoDialog.cpp, the splash screen and the About box). The lines cited are those of the file before
    // the flattening, when the sheets were the strings of its constructor.
    // ------------------------------------------------------------------------------

    // The text colour of the copyright notice, a rule of the dialog's sheet: `QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; }`
    // (StartupLogoDialog.cpp:55 in the splash, :65 in the About box, before the flattening). TOKENIZE: StartupCopyrightNoticeText
    inline constexpr QColor StartupCopyrightNoticeText(0xAA, 0xAA, 0xAA);

    // The text size of the copyright notice, the same rule (StartupLogoDialog.cpp:55, :65 before the flattening).
    // TOKENIZE: StartupCopyrightNoticeSize
    inline constexpr int StartupCopyrightNoticeSize = 9;

    // The text colour of the link of the About box (Terms of Use), a rule of its sheet:
    // `QLabel#link { text-decoration: underline; color: #94D2FF; }` (StartupLogoDialog.cpp:66 before the flattening).
    // TOKENIZE: StartupLinkText
    inline constexpr QColor StartupLinkText(0x94, 0xD2, 0xFF);

    // ------------------------------------------------------------------------------
    // The text of the sheets the legacy GUI sets (StyleManager::setLegacyStyleSheet), byte for byte as the forms had them
    // ------------------------------------------------------------------------------

    //! `QFrame { color : rgb(34, 34, 34);}` (AzAssetBrowserWindow.ui:132, :206)
    inline QString AssetBrowserCreateDividerSheet()
    {
        return QStringLiteral("QFrame { color : rgb(%1, %2, %3);}")
            .arg(AssetBrowserCreateDividerLine.red())
            .arg(AssetBrowserCreateDividerLine.green())
            .arg(AssetBrowserCreateDividerLine.blue());
    }

    //! The sheet of the thumbnail view (AzAssetBrowserWindow.ui:567)
    inline QString AssetBrowserThumbnailViewSheet()
    {
        return QStringLiteral(
                   "AzToolsFramework--AssetBrowser--AssetBrowserThumbnailView, AzQtComponents--AssetFolderThumbnailView "
                   "{ background-color: %1; }")
            .arg(AssetBrowserThumbnailsFill.name());
    }

    //! The sheet of the python builder frame (ImporterRootDisplay.ui:79)
    inline QString AssetImporterPythonBuilderSheet()
    {
        return QStringLiteral(
                   "QFrame#m_pythonBuilderLayout {background-color: %1; margin-bottom: 6px; padding-top: 2px; padding-bottom: 2px; "
                   "border-top-left-radius: 2px; border-top-right-radius: 2px;}")
            .arg(AssetImporterPythonBuilderFill.name());
    }

    //! The sheet of the splash screen (StartupLogoDialog.cpp:54-55 before the flattening): the rules as they were, without the spaces of
    //! the line continuation between them, which the sheet parser does not read
    inline QString StartupLogoLoadingSheet()
    {
        return QStringLiteral(
                   "QLabel { background: transparent; color: 'white' } "
                   "QLabel#copyrightNotice { color: %1; font-size: %2px; } ")
            .arg(StartupCopyrightNoticeText.name())
            .arg(StartupCopyrightNoticeSize);
    }

    //! The sheet of the About box (StartupLogoDialog.cpp:65-66 before the flattening), with the same exception
    inline QString StartupLogoAboutSheet()
    {
        return QStringLiteral(
                   "QLabel#copyrightNotice { color: %1; font-size: %2px; } "
                   "QLabel#link { text-decoration: underline; color: %3; }")
            .arg(StartupCopyrightNoticeText.name())
            .arg(StartupCopyrightNoticeSize)
            .arg(StartupLinkText.name());
    }
} // namespace EditorFlatValues
