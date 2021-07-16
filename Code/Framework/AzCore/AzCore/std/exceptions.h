/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_EXCEPTIONS_H
#define AZSTD_EXCEPTIONS_H 1

#ifdef AZSTD_HAS_EXEPTIONS

#define AZSTD_TRY_BEGIN try {
#define AZSTD_CATCH(x)  } catch (x) {
#define AZSTD_CATCH_ALL } catch (...) {
#define AZSTD_CATCH_END }

#define AZSTD_RAISE(x)  throw (x)
#define AZSTD_RERAISE   throw

#define AZSTD_THROW0()  throw ()
#define AZSTD_THROW1(x) throw (...)

#define AZSTD_THROW(x, y)   throw x(y)
#define AZSTD_THROW_NCEE(x, y)  _THROW(x, y)

#else // AZSTD_HAS_EXEPTIONS

#define AZSTD_TRY_BEGIN { \
        {
#define AZSTD_CATCH(x)  } if (0) {
#define AZSTD_CATCH_ALL } if (0) {
#define AZSTD_CATCH_END } \
    }

#define AZSTD_RAISE(x)  ::std:: _Throw(x)
#define AZSTD_RERAISE

#define AZSTD_THROW0()
#define AZSTD_THROW1(x)
#define AZSTD_THROW(x, y)   x(y)._Raise()
#define AZSTD_THROW_NCEE(x, y)  _THROW(x, y)

#endif //

#endif // AZSTD_EXCEPTIONS_H
#pragma once



