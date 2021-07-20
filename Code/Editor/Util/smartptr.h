/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_SMARTPTR_H
#define CRYINCLUDE_EDITOR_UTIL_SMARTPTR_H
#pragma once

#include <CryCommon/smartptr.h>

#define TSmartPtr _smart_ptr

/** Use this to define smart pointers of classes.
        For example:
        class CNode : public CRefCountBase {};
        SMARTPTRTypeYPEDEF( CNode );
        {
            CNodePtr node; // Smart pointer.
        }
*/

#define SMARTPTR_TYPEDEF(Class) typedef TSmartPtr<Class> Class##Ptr

#endif // CRYINCLUDE_EDITOR_UTIL_SMARTPTR_H
