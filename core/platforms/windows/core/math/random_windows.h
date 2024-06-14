#ifndef V_FRAMEWORK_CORE_PLATFORM_WINDOWS_MATH_RANDOM_WINDOWS_H
#define V_FRAMEWORK_CORE_PLATFORM_WINDOWS_MATH_RANDOM_WINDOWS_H

#include <core/base.h>

namespace V {
    /**
     * Provides a better yet more computationally costly random implementation.
     * To be used in less frequent scenarios, i.e. to get a good seed.
     */
    class BetterPseudoRandom_Windows
    {
    public:
        BetterPseudoRandom_Windows();
        ~BetterPseudoRandom_Windows();

        /// Fills a buffer with random data, if true is returned. Otherwise the random generator fails to generate random numbers.
        bool GetRandom(void* data, size_t dataSize);

        /// Template helper for value types \ref GetRandom
        template<class T>
        bool GetRandom(T& value)    { return GetRandom(&value, sizeof(T)); }

    private:
        V::u64 m_generatorHandle;
    };

    using BetterPseudoRandom_Platform = BetterPseudoRandom_Windows;
}

#endif // V_FRAMEWORK_CORE_PLATFORM_WINDOWS_MATH_RANDOM_WINDOWS_H