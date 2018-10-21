// -*- mode: c++; -*-

#ifndef POF_ALGORITHM_HH
#define POF_ALGORITHM_HH

template< typename InputIterator, typename OutputIterator, typename Predicate >
inline void
copy_until (InputIterator iter, InputIterator last, OutputIterator out,
            Predicate pred) {
    for (; iter != last && pred (*iter); ++iter) {
        *out = *iter;
    }
}

template< typename InputIterator, typename OutputIterator >
inline void
copy_until_zero (InputIterator iter, InputIterator last, OutputIterator out) {
    copy_until (iter, last, out, [](auto c) { return bool (c); });
}

#endif // POF_ALGORITHM_HH
