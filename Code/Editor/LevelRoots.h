/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_LEVELROOTS_H
#define CRYINCLUDE_EDITOR_LEVELROOTS_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QVector>
#endif

namespace LevelRoots
{
    //----------------------------------------------------------------------
    // Description of a single source root that the editor treats as a
    // "Levels" container.
    //
    // The project root is always present (even when its Levels folder does
    // not yet exist on disk), so that "Save As" / "New Level" can create
    // the first level for a fresh project. Gem roots are added only when
    // the corresponding folder actually exists on disk.
    //----------------------------------------------------------------------
    struct Root
    {
        QString displayName;   // "Levels" for the project, "<GemName>/Levels" for a gem
        QString absolutePath;  // absolute path to the Levels folder on disk
        bool isProject = false;
    };

    //----------------------------------------------------------------------
    // Determines which active gems are surfaced as roots.
    //   ExistingOnly  - only gems whose "Assets/Levels" folder already
    //                   exists on disk. Right for open / browse surfaces.
    //   AllActive     - every active gem, even if its "Assets/Levels"
    //                   folder is empty or missing. Right for save / new
    //                   surfaces, where the folder will be created on
    //                   demand by CFileUtil::CreatePath.
    //----------------------------------------------------------------------
    enum class Mode
    {
        ExistingOnly,
        AllActive
    };

    //----------------------------------------------------------------------
    // Returns the project Levels root followed by every active gem that
    // matches the requested mode. The list ordering is stable and safe to
    // use as the order of top-level items in a tree view.
    //----------------------------------------------------------------------
    QVector<Root> Enumerate(Mode mode = Mode::ExistingOnly);

    //----------------------------------------------------------------------
    // Returns true when absolutePath sits anywhere underneath the project
    // root or any active gem's source path. Used to decide whether a
    // recent-file entry should be visible in the launch screen / Open
    // Recent menu (so gem-rooted levels are not filtered out).
    //----------------------------------------------------------------------
    bool IsPathUnderActiveSource(const QString& absolutePath);

    //----------------------------------------------------------------------
    // Returns a short, user-facing form of an absolute level file path.
    //   - Project file:    relative to the project (e.g. "Levels/MyLevel/MyLevel.prefab").
    //   - Gem file under <gem>/Assets/Levels/...: rewritten as
    //                      "<GemName>/Levels/MyLevel/MyLevel.prefab"
    //                      (the "Assets/" segment is dropped to match the
    //                      naming convention used elsewhere in the editor).
    //   - Gem file outside Assets/Levels: "<GemName>/<gemRelative>".
    //   - Anything else:   the absolutePath, unchanged.
    // Always uses native separators in the result.
    //----------------------------------------------------------------------
    QString FormatRelativeDisplay(const QString& absolutePath);
}

#endif // CRYINCLUDE_EDITOR_LEVELROOTS_H
