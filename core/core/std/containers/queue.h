/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_CONTAINERS_QUEUE_H
#define V_FRAMEWORK_CORE_STD_CONTAINERS_QUEUE_H


#include <core/std/containers/deque.h>
#include <core/std/containers/vector.h>
#include <core/std/functional_basic.h>
#include <queue>

namespace VStd {
    template<class T, class Container = VStd::deque<T>>
    using queue = std::queue<T, Container>;
    template<class T, class Container = VStd::vector<T>, class Compare = VStd::less<typename Container::value_type>>
    using priority_queue = std::priority_queue<T, Container, Compare>;
}

#endif // V_FRAMEWORK_CORE_STD_CONTAINERS_QUEUE_H