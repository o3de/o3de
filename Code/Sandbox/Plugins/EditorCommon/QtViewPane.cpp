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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"
#include "platform.h"

#pragma warning(disable: 4266) // disabled warning from afk overrides

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <afxwin.h>
#include <vector>

#include "QtViewPane.h"

#include "Include/IViewPane.h"
#include "Util/RefCountBase.h"
#include "QtWinMigrate/qwinwidget.h"

#include <QWidget>
#include <QEvent>
#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>

#include "QtUtil.h"

// ugly dependencies:
#pragma warning(push)
#pragma warning(disable: 4244) // warning C4244: 'argument' : conversion from 'A' to 'B', possible loss of data
#include "Functor.h"
class CXmlArchive;
#include <IRenderer.h>
#include "Util/PathUtil.h"
#pragma warning(pop)
// ^^^

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

