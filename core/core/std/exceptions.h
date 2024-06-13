/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_EXCEPTIONS_H
#define V_FRAMEWORK_CORE_STD_EXCEPTIONS_H

#ifdef VSTD_HAS_EXEPTIONS

#define VSTD_TRY_BEGIN try {
#define VSTD_CATCH(x)  } catch (x) {
#define VSTD_CATCH_ALL } catch (...) {
#define VSTD_CATCH_END }

#define VSTD_RAISE(x)  throw (x)
#define VSTD_RERAISE   throw

#define VSTD_THROW0()  throw ()
#define VSTD_THROW1(x) throw (...)

#define VSTD_THROW(x, y)   throw x(y)
#define VSTD_THROW_NCEE(x, y)  _THROW(x, y)

#else // VSTD_HAS_EXEPTIONS

#define VSTD_TRY_BEGIN { \
        {
#define VSTD_CATCH(x)  } if (0) {
#define VSTD_CATCH_ALL } if (0) {
#define VSTD_CATCH_END } \
    }

#define VSTD_RAISE(x)  ::std:: _Throw(x)
#define VSTD_RERAISE

#define VSTD_THROW0()
#define VSTD_THROW1(x)
#define VSTD_THROW(x, y)   x(y)._Raise()
#define VSTD_THROW_NCEE(x, y)  _THROW(x, y)

#endif 

#endif // V_FRAMEWORK_CORE_STD_EXCEPTIONS_H