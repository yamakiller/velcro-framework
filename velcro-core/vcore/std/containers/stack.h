/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_CONTAINERS_STACK_H
#define V_FRAMEWORK_CORE_STD_CONTAINERS_STACK_H

#include <vcore/std/containers/deque.h>
#include <stack>

namespace VStd {
    template<class T, class Container = VStd::deque<T>>
    using stack = std::stack<T, Container>;
}

#endif // V_FRAMEWORK_CORE_STD_CONTAINERS_STACK_H