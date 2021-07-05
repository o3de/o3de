/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QValidator>
#endif

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace O3DE::ProjectManager
{
    class PathValidator 
        : public QValidator
    {
    public:
        enum class PathMode {
            ExistingFile,   //!< A single, existings file. Useful for "Open file"
            ExistingFolder, //!< A single, existing directory. Useful for "Open Folder"
            AnyFile         //!< A single, valid file, doesn't have to exist but the directory must.  Useful for "Save File"
        };

        explicit PathValidator(PathMode pathMode, QWidget* parent = nullptr);
        ~PathValidator() = default;

        void setAllowEmpty(bool allowEmpty);
        void setPathMode(PathMode pathMode);

        QValidator::State validate(QString &text, int &) const override;

    private:
        PathMode m_pathMode = PathMode::AnyFile;
        bool m_allowEmpty = false;
    };
} // namespace O3DE::ProjectManager
