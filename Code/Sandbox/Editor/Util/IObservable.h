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

// Description : Handy macro when working with observers in interfaces


#ifndef CRYINCLUDE_EDITOR_UTIL_IOBSERVABLE_H
#define CRYINCLUDE_EDITOR_UTIL_IOBSERVABLE_H
#pragma once
#define DEFINE_OBSERVABLE_PURE_METHODS(observerClassName)               \
    virtual bool RegisterObserver(observerClassName * pObserver) = 0;   \
    virtual bool UnregisterObserver(observerClassName * pObserver) = 0; \
    virtual void UnregisterAllObservers() = 0;
#endif // CRYINCLUDE_EDITOR_UTIL_IOBSERVABLE_H
