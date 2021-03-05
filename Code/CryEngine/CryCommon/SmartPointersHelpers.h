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

// This header is provided for backwards compatibility. Avoid including this header, instead
// include the needed smart ptr headers

#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

#define DECLARE_SMART_POINTERS(name)                      \
    typedef AZStd::shared_ptr<name> name##Ptr;            \
    typedef AZStd::shared_ptr<const name> name##ConstPtr; \
    typedef AZStd::weak_ptr<name> name##WeakPtr;          \
    typedef AZStd::weak_ptr<const name> name##ConstWeakPtr;
