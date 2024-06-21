SET(FILES
    vcore/base.h
    vcore/platform_default.h
    vcore/platform_incl.h
    vcore/platform_restricted_file_default.h
    vcore/platform.h
    vcore/platform.cc    
    vcore/variadic.h
    vcore/casting/lossy_cast.h
    vcore/casting/numeric_cast_internal.h
    vcore/casting/numeric_cast.h
    vcore/compression/compression.h
    vcore/compression/compression.cc 
    vcore/compression/zstd_compression.h 
    vcore/compression/zstd_compression.cc
    vcore/console/console_data_wrapper.h
    vcore/console/console_functor.h
    vcore/console/console_functor.cc
    vcore/console/console_type_helpers.h
    vcore/console/console.h
    vcore/console/console.cc
    vcore/console/iconsole_types.h
    vcore/console/iconsole.h
    vcore/console/ilogger.h
    vcore/debug/budget_tracker.h
    vcore/debug/budget_tracker.cc
    vcore/debug/budget.h
    vcore/debug/budget.cc
    vcore/debug/ievent_logger.h
    vcore/debug/memory_profiler.h
    vcore/debug/profiler_bus.h
    vcore/debug/profiler.h
    vcore/debug/profiler.cc
    vcore/debug/stack_tracer.h
    vcore/debug/time.h
    vcore/debug/trace_message_bus.h
    vcore/debug/trace_message_detector_bus.h
    vcore/debug/trace.h 
    vcore/debug/trace.cc
    vcore/detector/default_string_pool.h
    vcore/detector/detector_bus.h
    vcore/detector/detector_bus.cc
    vcore/detector/detector.h
    vcore/detector/detector.cc
    vcore/detector/stream.h
    vcore/detector/stream.cc
    vcore/event_bus/internal/bus_container.h
    vcore/event_bus/internal/call_stack_entry.h
    vcore/event_bus/internal/debug.h
    vcore/event_bus/internal/handlers.h
    vcore/event_bus/internal/storage_policies.h
    vcore/event_bus/bus_impl.h
    vcore/event_bus/environment.h
    vcore/event_bus/event_bus_environment.cc
    vcore/event_bus/event_bus_shared_dispatch_traits.h
    vcore/event_bus/event_bus.h
    vcore/event_bus/event.h
    vcore/event_bus/ievent_scheduler.h
    vcore/event_bus/ordered_event.h
    vcore/event_bus/policies.h
    vcore/event_bus/results.h
    vcore/event_bus/scheduled_event_handle.h
    vcore/event_bus/scheduled_event_handle.cc
    vcore/event_bus/scheduled_event.h
    vcore/event_bus/scheduled_event.cc
    vcore/interface/interface.h
    vcore/io/path/path.h
    vcore/io/path/path.cc
    vcore/io/path/path_fwd.h
    vcore/io/file_io.h  
    vcore/io/file_io.cc
    vcore/io/generic_streams.h
    vcore/io/generic_streams.cc
    vcore/io/io_utils.h
    vcore/io/io_utils.cc
    vcore/io/system_file.h
    vcore/io/system_file.cc
    vcore/ipc/shared_memory_common.h 
    vcore/ipc/shared_memory.h
    vcore/ipc/shared_memory.cc
    vcore/math/internal/math_type.h
    vcore/math/internal/simd.h
    vcore/math/crc.h
    vcore/math/crc.cc
    vcore/math/random.h
    vcore/math/sha1.h
    vcore/math/guid.h
    vcore/math/math_utils.h
    vcore/math/type_id.h 
    vcore/math/uuid.h
    vcore/math/uuid.cc
    vcore/math/sfmt.h
    vcore/math/sfmt.cc
    vcore/memory/allocator_records.h
    vcore/memory/allocator_records.cc
    vcore/memory/allocator_base.h
    vcore/memory/allocator_base.cc
    vcore/memory/allocator_manager.h
    vcore/memory/allocator_manager.cc
    vcore/memory/allocator_override_shim.h
    vcore/memory/allocator_override_shim.cc
    vcore/memory/allocator_wrapper.h
    vcore/memory/config.h
    vcore/memory/heap_schema.h
    vcore/memory/heap_schema.cc
    vcore/memory/hpha_schema.h
    vcore/memory/hpha_schema.cc
    vcore/memory/malloc_schema.h
    vcore/memory/malloc_schema.cc
    vcore/memory/iallocator.h
    vcore/memory/iallocator.cc
    vcore/memory/memory.h
    vcore/memory/memory.cc
    vcore/memory/osallocator.h
    vcore/memory/osallocator.cc
    vcore/memory/platform_memory_instrumentation.h
    vcore/memory/simple_schema_allocator.h
    vcore/memory/system_allocator.h
    vcore/memory/system_allocator.cc
    vcore/module/dynamic_module_handle.h
    vcore/module/dynamic_module_handle.cc
    vcore/module/environment.h
    vcore/module/environment.cc
    vcore/name/internal/name_data.h
    vcore/name/internal/name_data.cc
    vcore/name/name_dictionary.h
    vcore/name/name_dictionary.cc
    vcore/name/name.h
    vcore/name/name.cc
    vcore/outcome/internal/outcome_storage.h
    vcore/outcome/outcome.h
    vcore/platform_id/platform_id.h
    vcore/platform_id/platform_id.cc
    vcore/rtti/type.h
    vcore/settings/command_line.h
    vcore/settings/command_line.cc
    vcore/settings/setting.h
    vcore/settings/setting.cc
    vcore/statistics/named_running_statistic.h
    vcore/statistics/running_statistic.h
    vcore/statistics/running_statistic.cc
    vcore/statistics/statistical_profiler_proxy.h
    vcore/statistics/statistical_profiler.h
    vcore/statistics/statistics_manager.h
    vcore/statistics/time_data_statistics_manager.h
    vcore/statistics/time_data_statistics_manager.cc
    vcore/std/bind/bind.h
    vcore/std/bind/mem_fn.h
    vcore/std/chrono/chrono.h
    vcore/std/chrono/clocks.h
    vcore/std/chrono/types.h
    vcore/std/containers/array.h
    vcore/std/containers/bitset.h
    vcore/std/containers/compressed_pair.h
    vcore/std/containers/deque.h
    vcore/std/containers/fixed_forward_list.h
    vcore/std/containers/fixed_list.h
    vcore/std/containers/fixed_unordered_map.h
    vcore/std/containers/fixed_unordered_set.h
    vcore/std/containers/fixed_vector.h
    vcore/std/containers/forward_list.h
    vcore/std/containers/intrusive_list.h
    vcore/std/containers/intrusive_set.h
    vcore/std/containers/intrusive_slist.h
    vcore/std/containers/list.h
    vcore/std/containers/map.h
    vcore/std/containers/node_handle.h
    vcore/std/containers/queue.h
    vcore/std/containers/rbtree.h
    vcore/std/containers/set.h
    vcore/std/containers/stack.h
    vcore/std/containers/unordered_map.h
    vcore/std/containers/unordered_set.h
    vcore/std/containers/variant_impl.h
    vcore/std/containers/variant.h
    vcore/std/containers/vector.h
    vcore/std/delegate/delegate.h
    vcore/std/delegate/delegate_bind.h
    vcore/std/delegate/delegate_fwd.h
    vcore/std/function/function_base.h
    vcore/std/function/function_fwd.h
    vcore/std/function/function_template.h
    vcore/std/function/identity.h
    vcore/std/function/invoke.h
    vcore/std/parallel/atomic.h
    vcore/std/parallel/combinable.h
    vcore/std/parallel/condition_variable.h
    vcore/std/parallel/conditional_variable.h
    vcore/std/parallel/config.h
    vcore/std/parallel/exponential_backoff.h
    vcore/std/parallel/lock.h
    vcore/std/parallel/mutex.h
    vcore/std/parallel/scoped_lock.h
    vcore/std/parallel/semaphore.h
    vcore/std/parallel/shared_mutex.h
    vcore/std/parallel/shared_spin_mutex.h
    vcore/std/parallel/spin_mutex.h
    vcore/std/parallel/thread.h
    vcore/std/smart_ptr/checked_delete.h
    vcore/std/smart_ptr/intrusive_base.h
    vcore/std/smart_ptr/intrusive_ptr.h
    vcore/std/smart_ptr/intrusive_refcount.h
    vcore/std/smart_ptr/make_shared.h
    vcore/std/smart_ptr/shared_count.h
    vcore/std/smart_ptr/shared_ptr.h
    vcore/std/smart_ptr/sp_convertible.h
    vcore/std/smart_ptr/unique_ptr.h
    vcore/std/smart_ptr/weak_ptr.h
    vcore/std/string/utf8/core.h
    vcore/std/string/utf8/unchecked.h
    vcore/std/string/alphanum.h
    vcore/std/string/alphanum.cc
    vcore/std/string/conversions.h
    vcore/std/string/fixed_string.h
    vcore/std/string/memory_to_ascii.h
    vcore/std/string/memory_to_ascii.cc
    vcore/std/string/osstring.h
    vcore/std/string/string_view.h
    vcore/std/string/string.h
    vcore/std/string/string.cc
    vcore/std/string/tokenize.h
    vcore/std/string/wildcard.h
    vcore/std/typetraits/internal/is_template_copy_constructible.h
    vcore/std/typetraits/internal/type_sequence_traits.h
    vcore/std/typetraits/add_const.h
    vcore/std/typetraits/add_cv.h
    vcore/std/typetraits/add_pointer.h
    vcore/std/typetraits/add_reference.h
    vcore/std/typetraits/add_volatile.h
    vcore/std/typetraits/aligned_storage.h
    vcore/std/typetraits/alignment_of.h
    vcore/std/typetraits/common_type.h
    vcore/std/typetraits/conditional.h
    vcore/std/typetraits/config.h
    vcore/std/typetraits/conjunction.h
    vcore/std/typetraits/decay.h
    vcore/std/typetraits/disjunction.h
    vcore/std/typetraits/extent.h
    vcore/std/typetraits/function_traits.h
    vcore/std/typetraits/has_member_function.h
    vcore/std/typetraits/has_virtual_destructor.h
    vcore/std/typetraits/integral_constant.h
    vcore/std/typetraits/intrinsics.h
    vcore/std/typetraits/invoke_traits.h
    vcore/std/typetraits/is_abstract.h
    vcore/std/typetraits/is_arithmetic.h
    vcore/std/typetraits/is_array.h
    vcore/std/typetraits/is_assignable.h
    vcore/std/typetraits/is_base_of.h
    vcore/std/typetraits/is_class.h
    vcore/std/typetraits/is_compound.h
    vcore/std/typetraits/is_const.h
    vcore/std/typetraits/is_constructible.h
    vcore/std/typetraits/is_convertible.h
    vcore/std/typetraits/is_destructible.h
    vcore/std/typetraits/is_empty.h
    vcore/std/typetraits/is_enum.h
    vcore/std/typetraits/is_floating_point.h
    vcore/std/typetraits/is_function.h
    vcore/std/typetraits/is_fundamental.h
    vcore/std/typetraits/is_integral.h
    vcore/std/typetraits/is_lvalue_reference.h
    vcore/std/typetraits/is_member_function_pointer.h
    vcore/std/typetraits/is_member_object_pointer.h
    vcore/std/typetraits/is_member_pointer.h
    vcore/std/typetraits/is_object.h
    vcore/std/typetraits/is_pod.h
    vcore/std/typetraits/is_pointer.h
    vcore/std/typetraits/is_polymorphic.h
    vcore/std/typetraits/is_reference.h
    vcore/std/typetraits/is_rvalue_reference.h
    vcore/std/typetraits/is_same.h
    vcore/std/typetraits/is_scalar.h
    vcore/std/typetraits/is_signed.h
    vcore/std/typetraits/is_swappable.h
    vcore/std/typetraits/is_trivial.h
    vcore/std/typetraits/is_trivially_copyable.h
    vcore/std/typetraits/is_union.h
    vcore/std/typetraits/is_unsigned.h
    vcore/std/typetraits/is_void.h
    vcore/std/typetraits/is_unsigned.h
    vcore/std/typetraits/is_void.h
    vcore/std/typetraits/is_volatile.h
    vcore/std/typetraits/negation.h
    vcore/std/typetraits/rank.h
    vcore/std/typetraits/remove_all_extents.h
    vcore/std/typetraits/remove_const.h
    vcore/std/typetraits/remove_cv.h
    vcore/std/typetraits/remove_cvref.h
    vcore/std/typetraits/remove_extent.h
    vcore/std/typetraits/remove_pointer.h
    vcore/std/typetraits/remove_reference.h
    vcore/std/typetraits/remove_volatile.h
    vcore/std/typetraits/static_storage.h
    vcore/std/typetraits/tuple_traits.h
    vcore/std/typetraits/type_id.h
    vcore/std/typetraits/type_identity.h
    vcore/std/typetraits/typetraits.h
    vcore/std/typetraits/underlying_type.h
    vcore/std/typetraits/void_t.h
    vcore/std/algorithm.h
    vcore/std/allocator_ref.h
    vcore/std/allocator_static.h
    vcore/std/allocator_traits.h
    vcore/std/allocator.h
    vcore/std/allocator.cc
    vcore/std/base.h
    vcore/std/config.h
    vcore/std/createdestroy.h
    vcore/std/exceptions.h
    vcore/std/functional_basic.h
    vcore/std/functional.h
    vcore/std/hash_table.h
    vcore/std/hash.h
    vcore/std/hash.cc
    vcore/std/iterator.h
    vcore/std/limits.h
    vcore/std/math.h
    vcore/std/numeric.h
    vcore/std/optional.h
    vcore/std/ratio.h
    vcore/std/reference_wrapper.h
    vcore/std/time.h
    vcore/std/tuple.h
    vcore/std/utils.h
    vcore/string_func/string_func.h
    vcore/string_func/string_func.cc
    vcore/threading/thread_safe_deque.h
    vcore/threading/thread_safe_object.h
    vcore/time/itime.h
    vcore/unit_test/mocks/mock_file_io_base.h
    vcore/unit_test/test_types.h
    vcore/unit_test/unit_test.h
    vcore/utilitys/city.h
    vcore/utilitys/city.cc
    vcore/utilitys/citycrc.h
    vcore/utilitys/type_hash.h
    vcore/utilitys/type_hash.cc
    vcore/utilitys/utility.h
    vcore/utilitys/utility.cc
)