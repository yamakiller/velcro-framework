#include <core/std/hash.h>
#include <core/std/algorithm.h>

namespace VStd {
      static constexpr VStd::size_t prime_list[] = {
        7ul,          23ul,
        53ul,         97ul,         193ul,       389ul,       769ul,
        1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
        49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
        1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
        50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
        1610612741ul, 3221225473ul, 4294967291ul
    };

    // Bucket size suitable to hold n elements.
    VStd::size_t hash_next_bucket_size(VStd::size_t n) {
        const VStd::size_t* first = prime_list;
        const VStd::size_t* last =  prime_list + V_ARRAY_SIZE(prime_list);
        const VStd::size_t* pos = VStd::lower_bound(first, last, n);
        return (pos == last ? *(last - 1) : *pos);
    }
}