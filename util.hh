// -*- mode: c++; -*-

#ifndef POF_UTIL_HH
#define POF_UTIL_HH

#include <string>

inline bool
in_set (const char c, const char* x = "") {
    if (0 == x || 0 == x [0]) return false;
    for (; x [0] && c != x [0]; ++x) ;
    return x [0];
}

inline string
string_from (int id) {
    string s;

    const char* p = reinterpret_cast< const char* > (&id);
    copy (p, p + sizeof id, back_inserter (s));

    return reverse (s.begin (), s.end ()), s;
}

#endif // POF_UTIL_HH
