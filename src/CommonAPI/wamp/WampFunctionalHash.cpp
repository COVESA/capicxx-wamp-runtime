#include <cassert>
#include <cstring>

#include <murmurhash/MurmurHash3.h>

#include <CommonAPI/Wamp/WampFunctionalHash.hpp>

/*
 * @see http://code.google.com/p/smhasher/
 */
#define SMHASHER_SEED_VALUE            0xc70f6907UL

namespace std {

size_t hash<pair<const char*, const char*> >::operator()(const pair<const char*, const char*>& t) const {
    const char* a = t.first;
    const char* b = t.second;

    assert(a);
    assert(b);

    uint32_t seed = static_cast<uint32_t>(SMHASHER_SEED_VALUE);
    MurmurHash3_x86_32(a, static_cast<int>(strlen(a)), seed, &seed);
    MurmurHash3_x86_32(b, static_cast<int>(strlen(b)), seed, &seed);

    return static_cast<size_t>(seed);
}

size_t hash<const char*>::operator()(const char* const t) const {
    uint32_t seed = static_cast<uint32_t>(SMHASHER_SEED_VALUE);
    MurmurHash3_x86_32(t, static_cast<int>(strlen(t)), seed, &seed);
    return static_cast<size_t>(seed);
}

size_t hash<pair<string, string> >::operator()(const pair<string, string>& t) const {
    const string& a = t.first;
    const string& b = t.second;

    uint32_t seed = static_cast<uint32_t>(SMHASHER_SEED_VALUE);
    MurmurHash3_x86_32(a.c_str(), static_cast<int>(a.length()), seed, &seed);
    MurmurHash3_x86_32(b.c_str(), static_cast<int>(b.length()), seed, &seed);

    return static_cast<size_t>(seed);
}

size_t hash<tuple<string, string, string> >::operator()(const tuple<string, string, string>& t) const {
    const string& a = get<0>(t);
    const string& b = get<1>(t);
    const string& c = get<2>(t);

    uint32_t seed = static_cast<uint32_t>(SMHASHER_SEED_VALUE);
    MurmurHash3_x86_32(a.c_str(), static_cast<int>(a.length()), seed, &seed);
    MurmurHash3_x86_32(b.c_str(), static_cast<int>(b.length()), seed, &seed);
    MurmurHash3_x86_32(c.c_str(), static_cast<int>(c.length()), seed, &seed);

    return static_cast<size_t>(seed);
}

size_t hash<tuple<string, string, string, bool> >::operator()(const tuple<string, string, string, bool>& t) const {
    const string& a = get<0>(t);
    const string& b = get<1>(t);
    const string& c = get<2>(t);
    const bool d = get<3>(t);

    uint32_t seed = static_cast<uint32_t>(SMHASHER_SEED_VALUE);
    MurmurHash3_x86_32(a.c_str(), static_cast<int>(a.length()), seed, &seed);
    MurmurHash3_x86_32(b.c_str(), static_cast<int>(b.length()), seed, &seed);
    MurmurHash3_x86_32(c.c_str(), static_cast<int>(c.length()), seed, &seed);
    MurmurHash3_x86_32(&d, sizeof(bool), seed, &seed);

    return static_cast<size_t>(seed);
}

size_t hash<tuple<string, string, string, int> >::operator()(const tuple<string, string, string, int>& t) const {
    const string& a = get<0>(t);
    const string& b = get<1>(t);
    const string& c = get<2>(t);
    const int d = get<3>(t);

    uint32_t seed = static_cast<uint32_t>(SMHASHER_SEED_VALUE);
    MurmurHash3_x86_32(a.c_str(), static_cast<int>(a.length()), seed, &seed);
    MurmurHash3_x86_32(b.c_str(), static_cast<int>(b.length()), seed, &seed);
    MurmurHash3_x86_32(c.c_str(), static_cast<int>(c.length()), seed, &seed);
    MurmurHash3_x86_32(&d, sizeof(d), seed, &seed);

    return static_cast<size_t>(seed);
}

size_t hash<tuple<string, string, string, string> >::operator()(const tuple<string, string, string, string>& t) const {
    const string& a = get<0>(t);
    const string& b = get<1>(t);
    const string& c = get<2>(t);
    const string& d = get<3>(t);

    uint32_t seed = static_cast<uint32_t>(SMHASHER_SEED_VALUE);
    MurmurHash3_x86_32(a.c_str(), static_cast<int>(a.length()), seed, &seed);
    MurmurHash3_x86_32(b.c_str(), static_cast<int>(b.length()), seed, &seed);
    MurmurHash3_x86_32(c.c_str(), static_cast<int>(c.length()), seed, &seed);
    MurmurHash3_x86_32(d.c_str(), static_cast<int>(d.length()), seed, &seed);

    return static_cast<size_t>(seed);
}

bool equal_to<pair<const char*, const char*> >::operator()(const pair<const char*, const char*>& a,
                                                           const pair<const char*, const char*>& b) const {
    if (a.first == b.first && a.second == b.second)
        return true;

    return !strcmp(a.first, b.first) && !strcmp(a.second, b.second);
}

}  // namespace std
