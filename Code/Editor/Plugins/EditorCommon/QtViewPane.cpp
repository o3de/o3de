/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "platform.h"

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
#include "Functor.h"
class CXmlArchive;
#include <IRenderer.h>
#include "Util/PathUtil.h"
// ^^^

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

