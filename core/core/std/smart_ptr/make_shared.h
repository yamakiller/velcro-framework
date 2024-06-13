#ifndef V_FRAMEWORK_CORE_STD_SMART_PTR_MAKE_SHARED_H
#define V_FRAMEWORK_CORE_STD_SMART_PTR_MAKE_SHARED_H

//  make_shared.hpp
//
//  Copyright (c) 2007, 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/make_shared.html
//  for documentation.

#include <core/std/smart_ptr/shared_ptr.h>
#include <core/std/typetraits/aligned_storage.h>

namespace VStd {
    namespace Internal {
        template< class T >
        class sp_ms_deleter {
        private:

            typedef typename VStd::aligned_storage< sizeof(T), ::VStd::alignment_of< T >::value >::type storage_type;

            bool initialized_;
            storage_type storage_;

        private:

            void destroy()  {
                if (initialized_)  {
                    reinterpret_cast< T* >(&storage_)->~T();
                    initialized_ = false;
                }
            }

        public:
            sp_ms_deleter()
                : initialized_(false) {
            }

            // optimization: do not copy storage_
            sp_ms_deleter(sp_ms_deleter const&)
                : initialized_(false) {
            }

            ~sp_ms_deleter() {
                destroy();
            }

            void operator()(T*) {
                destroy();
            }

            void* address() {
                return &storage_;
            }

            void set_initialized() {
                initialized_ = true;
            }
        };

        template< class T >
        T && sp_forward(T & t) {
            return static_cast< T && >(t);
        }
    } // namespace Internal

    // Zero-argument versions
    //
    // Used even when variadic templates are available because of the new T() vs new T issue

    template< class T >
    VStd::shared_ptr< T > make_shared() {
        VStd::shared_ptr< T > pt(static_cast< T* >(0), VStd::Internal::sp_inplace_tag< VStd::Internal::sp_ms_deleter< T > >());

        VStd::Internal::sp_ms_deleter< T >* pd = VStd::get_deleter< VStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T();
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        VStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return VStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A >
    VStd::shared_ptr< T > allocate_shared(A const& a) {
        VStd::shared_ptr< T > pt(static_cast< T* >(0), VStd::Internal::sp_inplace_tag< VStd::Internal::sp_ms_deleter< T > >(), a);

        VStd::Internal::sp_ms_deleter< T >* pd = VStd::get_deleter< VStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T();
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        VStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return VStd::shared_ptr< T >(pt, pt2);
    }

    // Variadic templates, rvalue reference
    template< class T, class ... Args >
    VStd::shared_ptr< T > make_shared(Args&& ... args) {
        VStd::shared_ptr< T > pt(static_cast< T* >(0), VStd::Internal::sp_inplace_tag<VStd::Internal::sp_ms_deleter< T > >());

        VStd::Internal::sp_ms_deleter< T >* pd = VStd::get_deleter< VStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new( pv ) T(VStd::Internal::sp_forward<Args>(args) ...);
        pd->set_initialized();

        T* pt2 = static_cast< T* >(pv);

        VStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return VStd::shared_ptr< T >(pt, pt2);
    }

    template< class T, class A, class Arg1, class ... Args >
    VStd::shared_ptr< T > allocate_shared(A const& a, Arg1&& arg1, Args&& ... args) {
        VStd::shared_ptr< T > pt(static_cast<T*>(0), VStd::Internal::sp_inplace_tag<VStd::Internal::sp_ms_deleter< T > >(), a);

        VStd::Internal::sp_ms_deleter< T >* pd = VStd::get_deleter< VStd::Internal::sp_ms_deleter< T > >(pt);

        void* pv = pd->address();

        ::new(pv)T(VStd::Internal::sp_forward<Arg1>(arg1), VStd::Internal::sp_forward<Args>(args) ...);
        pd->set_initialized();

        T* pt2 = static_cast<T*>(pv);

        VStd::Internal::sp_enable_shared_from_this(&pt, pt2, pt2);
        return VStd::shared_ptr< T >(pt, pt2);
    }

} // namespace VStd


#endif // V_FRAMEWORK_CORE_STD_SMART_PTR_MAKE_SHARED_H