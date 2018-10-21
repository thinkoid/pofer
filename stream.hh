// -*- mode: c++; -*-

#ifndef POF_STREAM_HH
#define POF_STREAM_HH

#include "util.hh"
#include "vector.hh"

template< typename T >
inline istream&
read (istream& s, T& t, size_t n = sizeof (T)) {
    t = T{ };
    return s.read (reinterpret_cast< char* > (&t), n);
}

static inline istream&
read (istream& s, char* pbuf, streamsize len, const char* x = "") {
    int n = 0;

    ASSERT (read (s, n));
    ASSERT (0 < n);

    char c = 0;
    streamsize j = 0;

    for (streamsize i = 0; i < n && j < len; ++i) {
        ASSERT (read (s, c));

        if (c && !in_set (c, x))
            pbuf [j++] = c;
    }

    return pbuf [j] = 0, s;
}

static inline istream&
read (istream& s, string& str, const char* x = "") {
    int n = 0;

    ASSERT (read (s, n));
    ASSERT (0 < n);

    char c = 0;

    for (size_t i = 0; i < size_t (n) && read (s, c); ++i) {
        if (c && !in_set (c, x))
            str += c;
    }

    return s;
}

template< typename T, size_t N >
inline istream&
read (istream& s, vector_t< T, N >& v) {
    for (size_t i = 0; i < N; ++i)
        ASSERT (read (s, v.value [i]));
    return s;
}

template< typename T >
inline istream&
read (istream& s, vector< T >& xs) {
    for (auto& x : xs) ASSERT (read (s, x));
    return s;
}

template< typename T >
inline ostream&
write (ostream& s, const T& t, size_t n = sizeof (T)) {
    return s.write (reinterpret_cast< const char* > (&t), n);
}

template< typename T >
static inline ostream&
write (ostream& s, const vector< T >& v) {
    return s.write (
        reinterpret_cast< const char* > (&v.data ()),
        v.size () * sizeof v [0]);
}

template< typename T, size_t N >
inline std::ostream&
operator<< (std::ostream& s, const vector_t< T, N >& v) {
    for (size_t i = 0; i < N; ++i)
        s << v.value [i] << " ";

    return s;
}

#endif // POF_STREAM_HH
