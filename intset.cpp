#include "intset.h"
#include <stdlib.h>
#include <cstring>
#include <stdexcept>

/* Note that these encodings are ordered, so:
 * INTSET_ENC_INT16 < INTSET_ENC_INT32 < INTSET_ENC_INT64. */
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

/* variants of the function doing the actual convertion only if the target
 * host is big endian */
#if (BYTE_ORDER == LITTLE_ENDIAN)
#define memrev16ifbe(p)
#define memrev32ifbe(p)
#define memrev64ifbe(p)
#define intrev16ifbe(v) (v)
#define intrev32ifbe(v) (v)
#define intrev64ifbe(v) (v)
#else
#define memrev16ifbe(p) memrev16(p)
#define memrev32ifbe(p) memrev32(p)
#define memrev64ifbe(p) memrev64(p)
#define intrev16ifbe(v) intrev16(v)
#define intrev32ifbe(v) intrev32(v)
#define intrev64ifbe(v) intrev64(v)
#endif

using namespace bRedis;

namespace
{
    /* Return the required encoding for the provided value. */
    inline uint8_t _intsetValueEncoding(int64_t v)
    {
        if (v < INT32_MIN || v > INT32_MAX)
            return INTSET_ENC_INT64;
        else if (v < INT16_MIN || v > INT16_MAX)
            return INTSET_ENC_INT32;
        else
            return INTSET_ENC_INT16;
    }

    /* Return the value at pos, given an encoding. */
    inline static int64_t _intsetGetEncoded(INTSET::intset *is, int pos, uint8_t enc)
    {
        int64_t v64;
        int32_t v32;
        int16_t v16;

        if (enc == INTSET_ENC_INT64)
        {
            memcpy(&v64, ((int64_t *)is->contents) + pos, sizeof(v64));
            memrev64ifbe(&v64);
            return v64;
        }
        else if (enc == INTSET_ENC_INT32)
        {
            memcpy(&v32, ((int32_t *)is->contents) + pos, sizeof(v32));
            memrev32ifbe(&v32);
            return v32;
        }
        else
        {
            memcpy(&v16, ((int16_t *)is->contents) + pos, sizeof(v16));
            memrev16ifbe(&v16);
            return v16;
        }
    }

    /* Return the value at pos, using the configured encoding. */
    inline static int64_t _intsetGet(INTSET::intset *is, int pos)
    {
        return _intsetGetEncoded(is, pos, intrev32ifbe(is->encoding));
    }

    /* Set the value at pos, using the configured encoding. */
    static void _intsetSet(INTSET::intset *is, int pos, int64_t value)
    {
        uint32_t encoding = intrev32ifbe(is->encoding);

        if (encoding == INTSET_ENC_INT64)
        {
            ((int64_t *)is->contents)[pos] = value;
            memrev64ifbe(((int64_t *)is->contents) + pos);
        }
        else if (encoding == INTSET_ENC_INT32)
        {
            ((int32_t *)is->contents)[pos] = value;
            memrev32ifbe(((int32_t *)is->contents) + pos);
        }
        else
        {
            ((int16_t *)is->contents)[pos] = value;
            memrev16ifbe(((int16_t *)is->contents) + pos);
        }
    }
}

void INTSET::resize(uint32_t len)
{
    uint32_t size = len * intrev32ifbe(is_->encoding);
    is_ = (intset *)realloc(is_, sizeof(intset) + size);
    if (is_ == nullptr)
        throw std::runtime_error("Failed to allocate memory");
}

std::tuple<bool, uint32_t> INTSET::search(int64_t value) const
{
    int min = 0, max = intrev32ifbe(is_->length) - 1, mid = -1;
    int64_t cur = -1;

    if (intrev32ifbe(is_->length) == 0)
        /* The value can never be found when the set is empty */
        return {false, 0};
    else
    {
        if (value > _intsetGet(is_, intrev32ifbe(is_->length) - 1))
            return {false, intrev32ifbe(is_->length)};
        else if (value < _intsetGet(is_, 0))
            return {false, 0};
    }

    // bineray search
    while (max >= min)
    {
        mid = ((unsigned int)min + (unsigned int)max) >> 1;
        cur = _intsetGet(is_, mid);
        if (value > cur)
            min = mid + 1;
        else if (value < cur)
            max = mid - 1;
        else
            break;
    }

    if (value == cur)
        return {true, mid};
    else
        return {false, min};
}

void INTSET::upgradeAndAdd(int64_t value)
{
    uint8_t curenc = intrev32ifbe(is_->encoding);
    uint8_t newenc = _intsetValueEncoding(value);
    int length = intrev32ifbe(is_->length);
    int prepend = value < 0 ? 1 : 0;

    /* First set new encoding and resize */
    is_->encoding = intrev32ifbe(newenc);
    resize(intrev32ifbe(is_->length) + 1);

    /* Upgrade back-to-front so we don't overwrite values.
     * Note that the "prepend" variable is used to make sure we have an empty
     * space at either the beginning or the end of the intset. */
    while (length--)
        _intsetSet(is_, length + prepend, _intsetGetEncoded(is_, length, curenc));

    /* Set the value at the beginning or the end. */
    if (prepend)
        _intsetSet(is_, 0, value);
    else
        _intsetSet(is_, intrev32ifbe(is_->length), value);
    is_->length = intrev32ifbe(intrev32ifbe(is_->length) + 1);
}

void INTSET::moveTail(uint32_t from, uint32_t to)
{
    void *src, *dst;
    uint32_t bytes = intrev32ifbe(is_->length) - from;
    uint32_t encoding = intrev32ifbe(is_->encoding);

    if (encoding == INTSET_ENC_INT64)
    {
        src = (int64_t *)is_->contents + from;
        dst = (int64_t *)is_->contents + to;
        bytes *= sizeof(int64_t);
    }
    else if (encoding == INTSET_ENC_INT32)
    {
        src = (int32_t *)is_->contents + from;
        dst = (int32_t *)is_->contents + to;
        bytes *= sizeof(int32_t);
    }
    else
    {
        src = (int16_t *)is_->contents + from;
        dst = (int16_t *)is_->contents + to;
        bytes *= sizeof(int16_t);
    }

    memmove(dst, src, bytes);
}

INTSET::INTSET()
    : is_(nullptr)
{
    is_ = (intset *)malloc(sizeof(intset));
    if (is_ == nullptr)
        throw std::runtime_error("Failed to allocate memory");
    is_->encoding = intrev32ifbe(INTSET_ENC_INT16);
    is_->length = 0;
}

INTSET::INTSET(const INTSET &in)
    : is_(nullptr)
{
    size_t len = in.bloLen();
    is_ = (intset *)malloc(len);
    if (is_ == nullptr)
        throw std::runtime_error("Failed to allocate memory");

    memcpy(is_, in.is_, len);
}

INTSET& INTSET::operator=(const INTSET &in)
{
    if(&in == this)
        return *this;
    
    size_t len = in.bloLen();
    intset* is__ = (intset *)malloc(len);
    if (is__ == nullptr)
        throw std::runtime_error("Failed to allocate memory");

    memcpy(is__, in.is_, len);

    free(is_);
    is_ = is__;

    return *this;
}

INTSET::INTSET(INTSET &&in)
    : is_(nullptr)
{
    is_ = in.is_;
    in.is_ = nullptr;
}

INTSET& INTSET::operator=(INTSET &&in)
{
    free(is_);
    is_ = in.is_;
    in.is_ = nullptr;

    return *this;
}

INTSET::~INTSET()
{
    if(is_)
        free(is_);
}

bool INTSET::add(int64_t value)
{
    uint8_t valenc = _intsetValueEncoding(value);
    uint32_t pos;

    if (valenc > intrev32ifbe(is_->encoding))
    {
        upgradeAndAdd(value);
        return true;
    }
    else
    {
        auto tp = search(value);
        bool bo = std::get<0>(tp);
        pos = std::get<1>(tp);
        if(bo)
            return false;
        
        resize(intrev32ifbe(is_->length) + 1);
        if (pos < intrev32ifbe(is_->length))
            moveTail(pos, pos +1);
    }

    _intsetSet(is_, pos, value);
    is_->length = intrev32ifbe(intrev32ifbe(is_->length)+1);

    return true;
}

bool INTSET::remove(int64_t value)
{
    uint8_t valenc = _intsetValueEncoding(value);
    bool scucess = false;

    auto [bo, pos] = search(value);
    if (valenc <= intrev32ifbe(is_->encoding) && bo)
    {
        uint32_t len = intrev32ifbe(is_->length);

        /* We know we can delete */
        scucess = true;

        /* Overwrite value with tail and update length */
        if (pos < (len - 1))
            moveTail(pos + 1, pos);
        resize(len - 1);
        is_->length = intrev32ifbe(len - 1);
    }
    return scucess;
}

bool INTSET::find(int64_t value) const
{
    uint8_t valenc = _intsetValueEncoding(value);
    return valenc <= intrev32ifbe(is_->encoding) && std::get<0>(search(value));
}

int64_t INTSET::random() const
{
    return _intsetGet(is_,rand()%intrev32ifbe(is_->length));
}

std::tuple<bool, int64_t> INTSET::get(uint32_t pos) const
{
    if(pos < intrev32ifbe(is_->length))
        return {true, _intsetGet(is_, pos)};
    return {false, 0};
}

uint32_t INTSET::len() const
{
    return intrev32ifbe(is_->length);
}

uint32_t INTSET::encoding() const
{
    return intrev32ifbe(is_->encoding);
}

size_t INTSET::bloLen() const
{
    return sizeof(intset)+intrev32ifbe(is_->length)*intrev32ifbe(is_->encoding);
}


#ifdef INTSET_TEST_MAIN
#include <sys/time.h>
#include <cassert>

long long usec(void) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000000)+tv.tv_usec;
}

INTSET createIntset(int bits, int size) {
    uint64_t mask = (1<<bits)-1;
    uint64_t i, value;
    INTSET is;

    for (i = 0; i < size; i++) {
        if (bits > 32) {
            value = (rand()*rand()) & mask;
        } else {
            value = rand() & mask;
        }
        is.add(value);
    }
    return is;
}

void ok(void) {
    printf("OK\n");
}

int main(int argc, char **argv) {

    printf("Value encodings: "); {
        assert(_intsetValueEncoding(-32768) == INTSET_ENC_INT16);
        assert(_intsetValueEncoding(+32767) == INTSET_ENC_INT16);
        assert(_intsetValueEncoding(-32769) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(+32768) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(-2147483648) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(+2147483647) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(-2147483649) == INTSET_ENC_INT64);
        assert(_intsetValueEncoding(+2147483648) == INTSET_ENC_INT64);
        assert(_intsetValueEncoding(-9223372036854775808ull) == INTSET_ENC_INT64);
        assert(_intsetValueEncoding(+9223372036854775807ull) == INTSET_ENC_INT64);
        ok();
    }

    printf("Basic adding: "); {
        INTSET intSet;
        bool success;
        
        success = intSet.add(5); assert(success);
        success = intSet.add(6); assert(success);
        success = intSet.add(4); assert(success);
        success = intSet.add(4); assert(!success);
        ok();
    }

    printf("Large number of random adds: "); {
        int inserts = 0;
        INTSET intSet;
        bool success;
        for (int i = 0; i < 1024; i++) {
            success = intSet.add(rand()%0x800);
            if (success) inserts++;
        }
        assert(intSet.len() == inserts);
        ok();
    }

    printf("Upgrade from int16 to int32: "); {
        INTSET is;
        is.add(32);
        assert(is.encoding() == INTSET_ENC_INT16);
        is.add(65535);
        assert(is.encoding() == INTSET_ENC_INT32);
        assert(is.find(32));
        assert(is.find(65535));
        // checkConsistency(is);

        INTSET is2;
        is2.add(32);
        assert(is2.encoding() == INTSET_ENC_INT16);
        is2.add(-65535);
        assert(is2.encoding() == INTSET_ENC_INT32);
        assert(is2.find(32));
        assert(is2.find(-65535));
        // checkConsistency(is);
        ok();
    }

    printf("Upgrade from int16 to int64: "); {
        INTSET is;
        is.add(32);
        assert(is.encoding() == INTSET_ENC_INT16);
        is.add(4294967295);
        assert(is.encoding() == INTSET_ENC_INT64);
        assert(is.find(32));
        assert(is.find(4294967295));
        // checkConsistency(is);

        INTSET is2;
        is2.add(32);
        assert(is2.encoding() == INTSET_ENC_INT16);
        is2.add(-4294967295);
        assert(is2.encoding() == INTSET_ENC_INT64);
        assert(is2.find(32));
        assert(is2.find(-4294967295));
        // checkConsistency(is);
        ok();
    }

    printf("Upgrade from int32 to int64: "); {
        INTSET is;
        is.add(65535);
        assert(is.encoding() == INTSET_ENC_INT32);
        is.add(4294967295);
        assert(is.encoding() == INTSET_ENC_INT64);
        assert(is.find(65535));
        assert(is.find(4294967295));
        // checkConsistency(is);

        INTSET is2;
        is2.add(65535);
        assert(is2.encoding() == INTSET_ENC_INT32);
        is2.add(-4294967295);
        assert(is2.encoding() == INTSET_ENC_INT64);
        assert(is2.find(65535));
        assert(is2.find(-4294967295));
        // checkConsistency(is);
        ok();
    }

    printf("Stress lookups: "); {
        long num = 100000, size = 10000;
        int i, bits = 20;
        long long start;
        INTSET is = createIntset(bits,size);
        
        start = usec();
        for (i = 0; i < num; i++)
            is.find(rand() % ((1<<bits)-1));
        printf("%ld lookups, %ld element set, %lldusec\n",num,size,usec()-start);
    }

    printf("Stress add+delete: "); {
        int i, v1, v2;
        INTSET is;
        for (i = 0; i < 0xffff; i++) {
            v1 = rand() % 0xfff;
            is.add(v1);
            assert(is.find(v1));

            v2 = rand() % 0xfff;
            is.remove(v2);
            assert(!is.find(v2));
        }
        // checkConsistency(is);
        ok();
    }

}
#endif





