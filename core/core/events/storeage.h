#ifndef V_FRAMEWORK_CORE_EVENTS_STOREAGESA_H
#define V_FRAMEWORK_CORE_EVENTS_STOREAGESA_H

namespace V {
    template <class Context>
    struct EventStreamGlobalStorageMode {
        /// @brief 返回这个 EventStream Data.
        /// @return 指向 EventStream 数据的指针.
        static Context* Get() {
            // Because the context in this policy lives in static memory space, and
            // doesn't need to be allocated, there is no reason to defer creation.
            return &GetOrCreate();
        }

        /// @brief 返回这个 EventStream Data.
        /// @return 对 EventStream 数据的引用.
        static Context& GetOrCreate() {
            static Context _context;
            return _context;
        }
    };

    template <class Context>
    struct EventStreamThreadLocalStorageMode {
        static Context* Get() {
            return &GetOrCreate();
        }
        
        static Context& GetOrCreate() {
            thread_local static Context _context;
            return _context;
        }
    };
}

#endif // V_FRAMEWORK_CORE_EVENTS_STOREAGESA_H