/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : forward declaration of TSerialize

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZEFWD_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZEFWD_H
#pragma once



template <class T>
class CSerializeWrapper;
struct ISerialize;
typedef CSerializeWrapper<ISerialize> TSerialize;

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZEFWD_H
