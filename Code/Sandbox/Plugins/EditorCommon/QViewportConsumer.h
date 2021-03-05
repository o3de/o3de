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

#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_QVIEWPORTCONSUMER_H
#define CRYINCLUDE_EDITORCOMMON_QVIEWPORTCONSUMER_H

struct SRenderContext;
struct SKeyEvent;
struct SMouseEvent;
class QKeySequence;

class QViewportConsumer
{
public:
    virtual ~QViewportConsumer() = default;
    virtual void OnViewportRender([[maybe_unused]] const SRenderContext& rc) {}

    // If you're overriding OnViewportKey, you should also override ProcessesViewportKey and return true if you're interested in a particular key.
    // If you don't, then registered shortcuts get keyPressed events first, and in many cases will never get passed to OnViewportKey
    virtual void OnViewportKey([[maybe_unused]] const SKeyEvent& ev) {}
    virtual bool ProcessesViewportKey([[maybe_unused]] const QKeySequence& key) { return false; }

    virtual void OnViewportMouse([[maybe_unused]] const SMouseEvent& ev) {}
};

#endif // CRYINCLUDE_EDITORCOMMON_QVIEWPORTCONSUMER_H
