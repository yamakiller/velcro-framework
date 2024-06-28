#ifndef V_FRAMEWORK_CORE_IO_STREAMER_STATISTICS_H
#define V_FRAMEWORK_CORE_IO_STREAMER_STATISTICS_H

#include <vcore/base.h>
#include <vcore/std/chrono/chrono.h>
#include <vcore/std/typetraits/is_arithmetic.h>
#include <vcore/std/string/string_view.h>

namespace V
{
    namespace IO
    {
        class Statistic
        {
        public:
            enum class Type
            {
                FloatingPoint,
                Integer,
                Percentage
            };

            static Statistic CreateFloat(VStd::string_view owner, VStd::string_view name, double value);
            static Statistic CreateInteger(VStd::string_view owner, VStd::string_view name, s64 value);
            static Statistic CreatePercentage(VStd::string_view owner, VStd::string_view name, double value);
            static void PlotImmediate(VStd::string_view owner, VStd::string_view name, double value);

            Statistic(const Statistic& rhs);
            Statistic(Statistic&& rhs);
            Statistic& operator=(const Statistic& rhs);
            Statistic& operator=(Statistic&& rhs);

            VStd::string_view GetOwner() const;
            VStd::string_view GetName() const;

            Type GetType() const;

            double GetFloatValue() const;
            s64 GetIntegerValue() const;
            double GetPercentage() const;

        private:
            Statistic() = default;

            VStd::string_view m_owner;
            VStd::string_view m_name;
            union
            {
                double m_floatingPoint;
                s64 m_integer;
            } m_value;
            Type m_type;
        };

        //! AverageWindow keeps track of the average of values in a sliding window.
        //! @StorageType The type of the value in the sliding window. Need to be a number of a VStd::chrono::duration.
        //! @AverageType The type CalculateAverage will return. StorageType needs to be able to be converted to AverageType.
        //! @WindowSize The maximum number of entries kept in the window.
        template<typename StorageType, typename AverageType, size_t WindowSize>
        class AverageWindow
        {
            static_assert(VStd::is_arithmetic<StorageType>::value || VStd::chrono::Internal::is_duration<StorageType>::value,
                "AverageWindow only support numbers and VStd::chrono::durations."); 
            static_assert((WindowSize & (WindowSize - 1)) == 0, "The WindowSize of AverageWindow needs to be a power of 2.");
        public:
            static const size_t s_windowSize = WindowSize;

            AverageWindow()
            {
                memset(&m_values, 0, sizeof(m_values));
                memset(&m_runningTotal, 0, sizeof(m_runningTotal));
            }

            //! Push a new entry into the window. If the window is full, the oldest value will be removed.
            void PushEntry(StorageType value)
            {
                size_t index = m_count & (WindowSize - 1);
                m_runningTotal += value;
                m_runningTotal -= m_values[index];
                m_values[index] = value;
                ++m_count;
            }

            //! Calculates the average value for the window.
            AverageType CalculateAverage() const
            {
                size_t count = m_count > 0 ? m_count : 1;
                return static_cast<AverageType>(static_cast<AverageType>(m_runningTotal) / static_cast<AverageType>(count < WindowSize ? count : WindowSize)); 
            }
            //! Gets the running total of the values in the window.
            StorageType GetTotal() const { return m_runningTotal; }
            //! Returns the total number of entries that have been passed in. This is not the total number of entries that are stored however.
            size_t GetNumRecorded() const { return m_count; }

        private:
            StorageType m_values[WindowSize];
            StorageType m_runningTotal;
            size_t m_count = 0;
        };

        using TimedAverageWindowDuration = VStd::chrono::microseconds;
        template<size_t WindowSize>
        using TimedAverageWindow = AverageWindow<TimedAverageWindowDuration, TimedAverageWindowDuration, WindowSize>;
        
        template<size_t WindowSize>
        class TimedAverageWindowScope
        {
        public:
            explicit TimedAverageWindowScope(TimedAverageWindow<WindowSize>& window)
                : m_window(window)
            {
                m_startTime = VStd::chrono::high_resolution_clock::now();
            }
            
            ~TimedAverageWindowScope()
            {
                VStd::chrono::high_resolution_clock::time_point now = VStd::chrono::high_resolution_clock::now();
                m_window.PushEntry(VStd::chrono::duration_cast<TimedAverageWindowDuration>(now - m_startTime));
            }

        private:
            TimedAverageWindow<WindowSize>& m_window;
            VStd::chrono::high_resolution_clock::time_point m_startTime;
        };

#define TIMED_AVERAGE_WINDOW_SCOPE(window) TimedAverageWindowScope<decltype(window)::s_windowSize> TIMED_AVERAGE_WINDOW##__COUNTER__ (window)

    } // namespace IO
} // namespace V

#endif // V_FRAMEWORK_CORE_IO_STREAMER_STATISTICS_H