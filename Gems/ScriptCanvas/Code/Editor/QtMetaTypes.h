/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QMetaType>
AZ_POP_DISABLE_WARNING

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Variable/VariableCore.h>
// VariableId is a UUID typedef for now. So we don't want to double reflect the UUID.
Q_DECLARE_METATYPE(AZ::Uuid);
Q_DECLARE_METATYPE(AZ::Data::AssetId);
Q_DECLARE_METATYPE(ScriptCanvas::Data::Type);
Q_DECLARE_METATYPE(ScriptCanvas::VariableId);
Q_DECLARE_METATYPE(ScriptCanvasEditor::SourceHandle);

