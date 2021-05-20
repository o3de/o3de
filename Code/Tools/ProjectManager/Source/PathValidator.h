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
#include <QValidator>
#endif


namespace O3DE::ProjectManager
{
    class PathValidator 
        : public QValidator
    {
    public:
        enum PathMode {
            ExistingFile,   //!< A single, existings file. Useful for "Open file"
            ExistingFolder, //!< A single, existing directory. Useful for "Open Folder"
            AnyFile         //!< A single, valid file, doesn't have to exist but the directory must.  Useful for "Save File"
        };

        explicit PathValidator(PathMode pathMode, QWidget* parent = nullptr);
        ~PathValidator() = default;

        void setAllowEmpty(bool allowEmpty);
        void setPathMode(PathMode pathMode);

        QValidator::State PathValidator::validate(QString &text, int &) const override;

    private:
        PathMode m_pathMode = PathMode::AnyFile;
        bool m_allowEmpty = false;
    };
} // namespace O3DE::ProjectManager
