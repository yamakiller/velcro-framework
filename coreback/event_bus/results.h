#ifndef V_FRAMEWORK_CORE_EVENT_BUS_RESULTS_H
#define V_FRAMEWORK_CORE_EVENT_BUS_RESULTS_H

#include <vcore/base.h>
#include <vcore/std/containers/vector.h>

/**
 * @file
 * Header file for structures that aggregate results returned by all
 * handlers of an EventBus event.
 */

namespace V
{
    /**
     * Aggregates results returned by all handlers of an EventBus event.
     * You can use this structure to add results, apply a logical AND
     * to results, and so on.
     *
     * @tparam T The output type of the aggregator.
     * @tparam Aggregator A function object that aggregates results.
     * The return type must match T.
     * For examples of function objects that you can use as aggregators,
     * see functional_basic.h.
     *
     * The following example sums the values returned by all handlers.
     * @code{.cpp}
     * EventBusReduceResult<int, VStd::plus<int>> result(0);
     * MyBus::BroadcastResult(result, &MyBus::Events::GetANumber);
     * V_Printf("%d", result.value);
     * @endcode
     *
     * The following example determines whether all handlers,
     * including the latest handler, return true.
     * @code{.cpp}
     * EventBusReduceResult<bool, VStd::logical_and<bool>> result(true);
     * MyBus::BroadcastResult(result, &MyBus::Events::IsDoneDoingThing);
     * // result.value is now only true if all handlers returned true.
     * V_Printf("%s", result.value ? "true" : "false");
     * @endcode
     *
     */
    template<class T, class Aggregator>
    struct EventBusReduceResult
    {
        /**
         * The current value, which new values will be aggregated with.
         */
        T value;

        /**
         * The function object that aggregates a new value with an existing value.
         */
        Aggregator unary;

        /**
         * Creates an instance of the class without setting an initial value or
         * a function object to use as the aggregator.
         */
        EventBusReduceResult() {}

        /**
         * Creates an instance of the class and sets the initial value and the function
         * object to use as the aggregator.
         * @param initialValue The initial value, which new values will be aggregated with.
         * @param aggregator A function object to aggregate the values.
         * For examples of function objects that you can use as aggregators,
         * see functional_basic.h.
         */
        EventBusReduceResult(const T& initialValue, const Aggregator& aggregator = Aggregator())
            : value(initialValue)
            , unary(aggregator)
        { }
        
        /**
         * Overloads the assignment operator to aggregate a new value with
         * the existing aggregated value.  Used ONLY when the return value of the function is const, or const&
         */
        void operator=(const T& rhs) { value = unary(value, rhs); }

        /**
         * Overloads the assignment operator to aggregate a new value with
         * the existing aggregated value using rvalue-ref to move
         */
        void operator=(T&& rhs) { value = unary(value, VStd::move(rhs)); }

        /**
         * Disallows copying an EventBusReduceResult object by reference.
         */
        EventBusReduceResult& operator=(const EventBusReduceResult&) = delete;
    };
    /**
     * Aggregates results returned by all handlers of an EventBus event.
     * Also puts the aggregated value into the input value that is
     * passed by reference. You can use this structure to add results,
     * apply a logical AND to results, and so on.
     *
     * @tparam T The output type of the aggregator.
     * @tparam Aggregator A function object that aggregates results.
     * The return type must match T.
     * For examples of function objects that you can use as aggregators,
     * see functional_basic.h.
     *
     * The following example replaces the value returned by a handler
     * with the sum of the value and previous handler return values.
     * @code{.cpp}
     * int myExistingInt = 5;
     * EventBusReduceResult<int&, VStd::plus<int>> result(myExistingInt);
     * MyBus::BroadcastResult(result, &MyBus::Events::GetANumber);
     * V_Printf("%d", result.value); // or V_Printf("%d", myExistingInt);
     * @endcode
     *
     * The following example determines whether all handlers,
     * including the latest handler, return true. Also replaces
     * the latest handler result with the aggregated result.
     * @code{.cpp}
     * bool myExistingBool = true;
     * EventBusReduceResult<bool&, VStd::logical_and<bool>> result(myExistingBool);
     * MyBus::BroadcastResult(result, &MyBus::Events::IsDoneDoingThing);
     * // myExistingBool is now only true if all handlers returned true.
     * V_Printf("%d", myExistingBool);
     * @endcode
     */
    template<class T, class Aggregator>
    struct EventBusReduceResult<T&, Aggregator>
    {
        /**
         * A reference to the current value, which new values will be aggregated with.
         */
        T& value;

        /**
         * The function object that aggregates a new value with an existing value.
         */
        Aggregator unary;

        /**
         * Creates an instance of the class, sets the reference to the initial value,
         * and sets the function object to use as the aggregator.
         * @param rhs A reference to the initial value, which new values will
         * be aggregated with.
         * @param aggregator A function object to aggregate the values.
         * For examples of function objects that you can use as aggregators,
         * see functional_basic.h.
         */
        explicit EventBusReduceResult(T& rhs, const Aggregator& aggregator = Aggregator())
            : value(rhs)
            , unary(aggregator)
        { }

        /**
         * Overloads the assignment operator to aggregate a new value with
         * the existing aggregated value using rvalue-ref
         */
        void operator=(T&& rhs)          { value = unary(value, VStd::move(rhs)); }

        /**
        * Overloads the assignment operator to aggregate a new value with
        * the existing aggregated value.  Used only when the return type is const, or const&
        */
        void operator=(const T& rhs) { value = unary(value, rhs); }


        /**
         * Disallows copying an EventBusReduceResult object by reference.
         */
        EventBusReduceResult& operator=(const EventBusReduceResult&) = delete;
    };

    /// @cond EXCLUDE_DOCS
    /**
     * @deprecated Use EventBusReduceResult instead.
     */
    template <class T, class Arithmetic>
    using EventBusArithmericResult = EventBusReduceResult<T, Arithmetic>;

    /**
     * @deprecated Use EventBusReduceResult instead.
     */
    template<class T, class Operator>
    using EventBusLogicalResult = EventBusReduceResult<T, Operator>;
    /// @endcond

    /**
     * Collects results returned by all handlers of an EventBus event.
     * The results are collected into an VStd::vector.
     *
     * @tparam T The return type of the handler.
     *
     * The following is an example of adding handler results to
     * a vector of previous results:
     * @code{.cpp}
     * EventBusAggregateResults<int> result;
     * MyBus::BroadcastResult(result, &MyBus::Events::GetANumber);
     * for (const int& val : result.values) { ... }
     * @endcode
     */
    template<class T>
    struct EventBusAggregateResults
    {
        /**
         * A vector that contains handler results.
         */
        VStd::vector<T> values;

        /**
         * Overloads the assignment operator to add a new result to a vector 
         * of previous results.
         * This const T& version is required to support const& returning functions.
         */
        void operator=(const T& rhs) { values.push_back(rhs); }
        
        /**
         * Overloads the assignment operator to add a new result to a vector 
         * of previous results, using rvalue-reference to move
         */
        void operator=(T&& rhs) { values.push_back(VStd::move(rhs)); }
    };
}

#endif // V_FRAMEWORK_CORE_EVENT_BUS_RESULTS_H