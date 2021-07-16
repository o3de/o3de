/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Handy macro when working with observers in interfaces


#ifndef CRYINCLUDE_EDITOR_UTIL_IOBSERVABLE_H
#define CRYINCLUDE_EDITOR_UTIL_IOBSERVABLE_H
#pragma once
#define DEFINE_OBSERVABLE_PURE_METHODS(observerClassName)               \
    virtual bool RegisterObserver(observerClassName * pObserver) = 0;   \
    virtual bool UnregisterObserver(observerClassName * pObserver) = 0; \
    virtual void UnregisterAllObservers() = 0;
#endif // CRYINCLUDE_EDITOR_UTIL_IOBSERVABLE_H
