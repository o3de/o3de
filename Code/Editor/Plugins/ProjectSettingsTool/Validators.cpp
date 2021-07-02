/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectSettingsTool_precompiled.h"
#include "Validators.h"

#include <QDir>
#include <QMimeDatabase>
#include <QRegularExpression>

#define STANDARD_SUCCESS ProjectSettingsTool::FunctorValidator::ReturnType(QValidator::Acceptable, "");

namespace 
{
    typedef ProjectSettingsTool::FunctorValidator::ReturnType RetType;

    static const int noMaxLength = -1;
    static const int maxIosVersionLength = 18;
    static const int androidPublicAppKeyLength = 392;
    static const char* xmlRelativePath = "Code/Tools/RC/Config/rc/";
    static const char* xmlMimeType = "application/xml";
    static const char* stringEmpty = "String is empty";

    // Returns true if string is valid android package and apple bundle identifier
    RetType RegularExpressionValidator(const QString& pattern, const QString& name, int maxLength = noMaxLength)
    {
        if (maxLength != noMaxLength && name.length() > maxLength)
        {
            return RetType(QValidator::Invalid, QObject::tr("Cannot be longer than %1 characters.")
                    .arg(QString::number(maxLength)));
        }
        else if (name.isEmpty())
        {
            return RetType(QValidator::Intermediate, QObject::tr(stringEmpty));
        }

        QRegularExpression regex(pattern);

        QRegularExpressionMatch match = regex.match(name, 0, QRegularExpression::PartialPreferCompleteMatch);

        if (match.hasMatch())
        {
            if (match.capturedLength(0) == name.length())
            {
                return STANDARD_SUCCESS;
            }
            else
            {
                return RetType(QValidator::Intermediate, "Input incorrect.");
            }
        }
        if (match.hasPartialMatch())
        {
            return RetType(QValidator::Intermediate, QObject::tr("Partially matches requirements."));
        }
        else
        {
            return RetType(QValidator::Invalid, QObject::tr("Fails to match requirements at all."));
        }
    }
}

namespace ProjectSettingsTool
{
    namespace Validators
    {
        namespace Internal
        {
            // Returns true if file is readable and the correct mime type
            RetType FileReadableAndCorrectType(const QString& path, const QString& fileType)
            {
                QDir dirPath(path);

                if (dirPath.isReadable())
                {
                    QMimeDatabase mimeDB;
                    QMimeType mimeType = mimeDB.mimeTypeForFile(path);

                    if (mimeType.name() == fileType)
                    {
                        return STANDARD_SUCCESS;
                    }
                    else
                    {
                        return RetType(QValidator::Intermediate, QObject::tr("File type should be %1, but is %2.")
                                .arg(fileType).arg(mimeType.name()));
                    }
                }
                else
                {
                    return RetType(QValidator::Intermediate, QObject::tr("File is not readable."));
                }
            }
        } // namespace Internal


        // Returns true if valid cross platform file or directory name
        RetType FileName(const QString& name)
        {
            // There was a known issue on android with '.' used in directory names
            // causing problems so it has been omitted from use
            return RegularExpressionValidator("[\\w,-]+", name);
        }

        RetType FileNameOrEmpty(const QString& name)
        {
            if (IsNotEmpty(name).first == QValidator::Acceptable)
            {
                return FileName(name);
            }
            else
            {
                return STANDARD_SUCCESS;
            }
        }

        // Returns true if string isn't empty
        RetType IsNotEmpty(const QString& value)
        {
            if (!value.isEmpty())
            {
                return STANDARD_SUCCESS;
            }
            else
            {
                return RetType(QValidator::Intermediate, QObject::tr(stringEmpty));
            }
        }

        // Returns true if string is valid as a boolean
        RetType BoolString(const QString& value)
        {
            if (value == "true" || value == "false")
            {
                return STANDARD_SUCCESS;
            }

            return RetType(QValidator::Invalid, QObject::tr("Invalid bool string."));
        }

        // Returns true if string is valid android package and apple bundle identifier
        RetType PackageName(const QString& name)
        {
            return RegularExpressionValidator("[a-zA-Z][A-Za-z0-9]*(\\.[a-zA-Z][A-Za-z0-9]*)+", name);
        }

        // Returns true if valid android version number
        RetType VersionNumber(const QString& value)
        {
            // Error handling built in already
            int ver = value.toInt();

            if (0 >= ver)
            {
                return RetType(QValidator::Invalid, QObject::tr("Version must be greater than 0."));
            }
            else if (ver > maxAndroidVersion)
            {
                return RetType(QValidator::Invalid, QObject::tr("Version must be less than or equal to %1.")
                        .arg(QString::number(maxAndroidVersion)));
            }
            else
            {
                return STANDARD_SUCCESS;
            }
        }

        // Returns true if valid ios version number
        RetType IOSVersionNumber(const QString& value)
        {
            return RegularExpressionValidator
                       ("(0|[1-9][0-9]{0,8}|[1-2][0-1][0-9]{0,8})(\\.(0|[1-9][0-9]{0,8}|[1-2][0-1][0-9]{0,8})){0,2}",
                       value,
                       maxIosVersionLength);
        }

        // Returns true if Public App Key is valid length
        RetType PublicAppKeyOrEmpty(const QString& value)
        {
            // If anyone knows that public app keys are not always 392 chars long
            // then this MUST be changed
            if (value.isEmpty() || value.length() == androidPublicAppKeyLength)
            {
                return STANDARD_SUCCESS;
            }
            else
            {
                return RetType(QValidator::Intermediate, QObject::tr("App key should be %1 characters long.")
                        .arg(QString::number(androidPublicAppKeyLength)));
            }
        }
        // Returns true if path is empty or a valid xml file relative to <build dir>
        RetType ValidXmlOrEmpty(const QString& path)
        {
            if (IsNotEmpty(path).first == QValidator::Acceptable)
            {
                return Internal::FileReadableAndCorrectType(xmlRelativePath + path, xmlMimeType);
            }
            else
            {
                return STANDARD_SUCCESS;
            }
        }

        // Returns true if path is empty or a valid png file
        RetType ValidPngOrEmpty(const QString& path)
        {
            if (IsNotEmpty(path).first == QValidator::Acceptable)
            {
                return Internal::FileReadableAndCorrectType(path, pngMimeType);
            }
            else
            {
                return STANDARD_SUCCESS;
            }
        }
    } // namespace Validators
} // namespace ProjectSettingsTool
