// -*- mode: c++; -*-

#ifndef POF_VECTOR_HH
#define POF_VECTOR_HH

template< typename T, size_t N >
struct vector_t {
    static constexpr size_t size = N;
    T value [N];
};

using vector3i_t = vector_t< int, 3 >;
using vector3f_t = vector_t< float, 3 >;

template< typename T, size_t N >
inline vector_t< T, N >&
operator+= (vector_t< T, N >& lhs, const vector_t< T, N >& rhs) {
    lhs.value [0] += rhs.value [0];
    lhs.value [1] += rhs.value [1];
    lhs.value [2] += rhs.value [2];

    return lhs;
}

template< typename T, size_t N >
inline vector_t< T, N >
operator+ (vector_t< T, N > lhs, const vector_t< T, N >& rhs) {
    return lhs += rhs, lhs;
}

using point3i_t = vector_t< int, 3 >;
using point3f_t = vector_t< float, 3 >;

template< typename T > struct vertex_t;

template< > struct vertex_t< float > { float x, y, z; };
using vertex3f_t = vertex_t< float >;

#endif // POF_VECTOR_HH
