/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_MEMORY_ALLOCATOR_WRAPPER_H
#define V_FRAMEWORK_CORE_MEMORY_ALLOCATOR_WRAPPER_H

#include <core/base.h>
#include <core/std/typetraits/alignment_of.h>
#include <core/std/typetraits/aligned_storage.h>
#include <core/std/utils.h>

namespace V {
    template<class Allocator>
    class AllocatorWrapper {
        public:
            using Descriptor = typename Allocator::Descriptor;
             AllocatorWrapper() {
        }

        ~AllocatorWrapper() {
            Destroy();
        }

        AllocatorWrapper(const Allocator&) = delete;

        /// Creates the wrapped allocator. You may pass any custom arguments to the allocator's constructor.
        template<typename... Args>
        void Create(const Descriptor& desc, Args&&... args) {
            Destroy();

            m_allocator = new (&m_storage) Allocator(AZStd::forward<Args>(args)...);
            m_allocator->Create(desc);
            m_allocator->PostCreate();
        }

        /// Destroys the wrapped allocator.
        void Destroy() {
            if (m_allocator) {
                m_allocator->PreDestroy();
                m_allocator->Destroy();
                m_allocator->~Allocator();
            }

            m_allocator = nullptr;
        }

        /// Provides access to the wrapped allocator.
        Allocator* Get() const {
            return m_allocator;
        }

        /// Provides access to the wrapped allocator.
        Allocator& operator*() const {
            return *m_allocator;
        }

        /// Provides access to the wrapped allocator.
        Allocator* operator->() const {
            return m_allocator;
        }

        /// Support for conversion to a boolean, returns true if the allocator has been created.
        operator bool() const {
            return m_allocator != nullptr;
        }

    private:
        Allocator* m_allocator = nullptr;
        typename VStd::aligned_storage<sizeof(Allocator), VStd::alignment_of<Allocator>::value>::type m_storage;
    };
}

#endif // V_FRAMEWORK_CORE_MEMORY_ALLOCATOR_WRAPPER_H