/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
