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

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QMetaType>
#include <QFont>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Styling/definitions.h>
#include <GraphCanvas/Editor/EditorTypes.h>

Q_DECLARE_METATYPE(QFont::Style);
Q_DECLARE_METATYPE(QFont::Capitalization);
Q_DECLARE_METATYPE(Qt::AlignmentFlag);
Q_DECLARE_METATYPE(Qt::PenCapStyle);
Q_DECLARE_METATYPE(Qt::PenStyle);
Q_DECLARE_METATYPE(GraphCanvas::Styling::ConnectionCurveType);
Q_DECLARE_METATYPE(GraphCanvas::Styling::PaletteStyle);
