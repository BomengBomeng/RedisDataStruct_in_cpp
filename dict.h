#ifndef BOMENG_REDIS_DICT_H
#define BOMENG_REDIS_DICT_H

#include <functional>

template <typename K, typename V, typename cmp = std::less<K>>
struct DICTentry
{
public:
    typedef K key_type;
    typedef V val_type;
    typedef cmp cmp_type;
public:
    K key;
    V val;
    DICTentry *next;

public:
    virtual unsigned int hashFunction() = 0;

public:
    DICTentry(K k, V v)
        : key(key), val(v), next(nullptr)
    {}
    virtual ~DICTentry(){ next=nullptr; }
    DICTentry(const DICTentry & entry)
        : key(entry.key), val(entry.val), next(nullptr)
    {}
    DICTentry& operator=(const DICTentry & entry)
    {
        key = entry.key;
        val = entry.val;
        next = nullptr;
    }
    DICTentry(DICTentry && entry) = delete;
    DICTentry& operator=(DICTentry && entry) = delete;
};

template <typename Entry>
class DICT
{
    static_assert(std::is_base_of<DICTentry<typename Entry::key_type, 
                                            typename Entry::val_type, 
                                            typename Entry::cmp_type>, 
                                  Entry>::value, "Entry must be derived from DICTentry");
private:
    struct DICTht{
        Entry **table;
        unsigned long size;
        unsigned long sizemask;
        unsigned long used;
    };

public:
    class iteratorP{
        long index;
        int table, safe;
        Entry *entry, *nextEntry;
        long long fingerprint;
    };

    class iterator : public iteratorP
    {
    };

    class const_iterator : public iteratorP
    {
    };
    

private:
    DICTht ht[2];
    long rehashidx;
    int iterators;
};

#endif