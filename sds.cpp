#include "sds.h"
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <iostream>

namespace
{
    const int SDS_LLSTR_SIZE = 21;

    int sdsll2str(char *s, long long value)
    {
        char *p, aux;
        unsigned long long v;
        size_t l;

        v = (value < 0) ? -value : value;
        p = s;
        do
        {
            *p++ = '0' + (v % 10);
            v /= 10;
        } while (v);

        if (value < 0)
            *p++ = '-';

        l = p - s;
        *p = '\0';

        p--;
        while (s < p)
        {
            aux = *s;
            *s = *p;
            *p = aux;
            s++;
            p--;
        }

        return l;
    }

    int sdsull2str(char *s, unsigned long long v)
    {
        char *p, aux;
        size_t l;

        p = s;
        do
        {
            *p++ = '0' + (v % 10);
            v /= 10;
        } while (v);

        l = p - s;
        *p = '\0';

        p--;
        while (s < p)
        {
            aux = *s;
            *s = *p;
            *p = aux;
            s++;
            p--;
        }
        return l;
    }
}

using namespace bRedis;

SDS::SDS(const void *init, size_t initlen)
    : sdshdr_(nullptr)
{
    SDSHDR *sh;
    if (init)
        sh = (SDSHDR *)malloc(sizeof(SDSHDR) + initlen + 1);
    else
        sh = (SDSHDR *)calloc(1, sizeof(SDSHDR) + initlen + 1);

    if (sh == NULL)
        throw std::runtime_error("Failed to allocate memory");

    sh->len = initlen;
    sh->free = 0;
    if (initlen && init)
        memcpy(sh->buf, init, initlen);

    sh->buf[initlen] = '\0';

    sdshdr_ = sh;
}

SDS::SDS(const char *init)
    : SDS(init, strlen(init))
{
}

SDS::SDS(void)
    : SDS("", 0)
{}

SDS::SDS(long long value)
    : sdshdr_(nullptr)
{
    char buf[SDS_LLSTR_SIZE];
    int len = sdsll2str(buf, value);

    *this = SDS(buf, len);
}

SDS::SDS(unsigned long long value)
    : sdshdr_(nullptr)
{
    char buf[SDS_LLSTR_SIZE];
    int len = sdsull2str(buf, value);

    *this = SDS(buf, len);
}

SDS::SDS(const SDS &sds)
    : sdshdr_(nullptr)
{
    SDSHDR *sh;
    if (sds.len())
        sh = (SDSHDR *)malloc(sizeof(SDSHDR) + sds.len() + sds.vail() + 1);
    else
        sh = (SDSHDR *)calloc(1, sizeof(SDSHDR) + sds.len() + sds.vail() + 1);

    if (sh == NULL)
        throw std::runtime_error("Failed to allocate memory");
    sh->len = sds.len();
    sh->free = sds.vail();
    if (sds.len())
        memcpy(sh->buf, sds.buf(), sds.len());

    sh->buf[sds.len()] = '\0';

    sdshdr_ = sh;
}

SDS &SDS::operator=(const SDS &sds)
{
    if (this == &sds)
        return *this;

    SDS t(sds);
    // 释放旧的
    free(sdshdr_);
    // 获取新的
    this->sdshdr_ = t.sdshdr_;
    t.sdshdr_ = nullptr;

    return *this;
}

SDS::SDS(SDS &&sds)
    : sdshdr_(nullptr)
{
    sdshdr_ = sds.sdshdr_;
    sds.sdshdr_ = nullptr;
}

SDS &SDS::operator=(SDS &&sds)
{
    if (this == &sds)
        return *this;
    
    free(sdshdr_);
    sdshdr_ = sds.sdshdr_;
    sds.sdshdr_ = nullptr;

    return *this;
}

SDS::~SDS(void)
{
    if (sdshdr_)
        free(sdshdr_);
    sdshdr_ = nullptr;
}

/*
std::tuple<SDS *, int> SDS::splitlen(const char *s, int len, const char *sep, int seplen)
{
    int elements = 0, slots = 5, start = 0;
    SDS *tokens = nullptr;

    if (seplen < 1 || len < 0)
        return std::make_tuple(nullptr, 0);

    tokens = (SDS *)malloc(sizeof(SDS) * slots);
    if (tokens == nullptr)
        throw std::runtime_error("Failed to allocate memory");

    if (len == 0)
        return std::make_tuple(tokens, 0);

    for (int j = 0; j < (len - (seplen - 1)); j++)
    {
        // make sure there is room for the next element and the final one
        if (slots < elements + 2)
        {
            SDS *newtokens;
            slots *= 2;
            newtokens = (SDS *)realloc(tokens, sizeof(SDS) * slots);
            if (newtokens == NULL)
                throw std::runtime_error("Failed to allocate memory");
            tokens = newtokens;
        }

        if ((seplen == 1 && *(s + j) == sep[0]) || (memcmp(s + j, sep, seplen) == 0))
        {
            tokens[elements++] = SDS(s + start, j - start);
            start = j + seplen;
            j = j + seplen - 1; // skip the separator
        }
    }

    // Add the final element. We are sure there is room in the tokens array.
    tokens[elements++] = SDS(s + start, len - start);

    return std::make_tuple(tokens, elements);
}
*/

std::vector<SDS> SDS::splitlen(const char *s, int len, const char *sep, int seplen)
{
    int elements = 0, slots = 5, start = 0;
    // SDS *tokens = nullptr;
    std::vector<SDS> tokens;

    if (seplen < 1 || len < 0)
        return tokens;

    // tokens = (SDS *)malloc(sizeof(SDS) * slots);
    // if (tokens == nullptr)
    //     throw std::runtime_error("Failed to allocate memory");

    if (len == 0)
        return tokens;

    for (int j = 0; j < (len - (seplen - 1)); j++)
    {
        /* make sure there is room for the next element and the final one */
        // if (slots < elements + 2)
        // {
        //     SDS *newtokens;
        //     slots *= 2;
        //     newtokens = (SDS *)realloc(tokens, sizeof(SDS) * slots);
        //     if (newtokens == NULL)
        //         throw std::runtime_error("Failed to allocate memory");
        //     tokens = newtokens;
        // }

        if ((seplen == 1 && *(s + j) == sep[0]) || (memcmp(s + j, sep, seplen) == 0))
        {
            // tokens[elements++] = SDS(s + start, j - start);
            tokens.emplace_back(s+start, j-start);
            start = j + seplen;
            j = j + seplen - 1; /* skip the separator */
        }
    }

    /* Add the final element. We are sure there is room in the tokens array. */
    // tokens[elements++] = SDS(s + start, len - start);
    tokens.emplace_back(s+start, len-start);

    return tokens;
}

/*
std::tuple<SDS*, int> SDS::splitargs(const char *line)
{
    const char*p = line;
    char *current = nullptr;
    char **vector = nullptr;

    int argc = 0;
    while(true)
    {
        // skip blanks
        while(*p && isspace(*p))
            p++;
        if(*p)
        {
            // get a token
            int inq = 0;
            int insq = 0;
            int done = 0;

            if(current == nullptr)
                current = SDS
        }
    }
}
*/

void SDS::growzero(size_t len)
{
    size_t totlen, curlen = sdshdr_->len;

    if (len <= curlen)
        return;

    this->MakeRoomFor(len - curlen);
    memset(sdshdr_->buf + curlen, 0, (len - curlen + 1));
    totlen = sdshdr_->len + sdshdr_->free;
    sdshdr_->len = len;
    sdshdr_->free = totlen - sdshdr_->len;
}

void SDS::cat(const void *t, size_t len)
{
    this->MakeRoomFor(len);
    memcpy(buf() + this->len(), t, len);
    sdshdr_->len += len;
    sdshdr_->free -= len;
    sdshdr_->buf[sdshdr_->len] = '\0';
}

void SDS::cat(const char *t)
{
    this->cat(t, strlen(t));
}

void SDS::cat(const SDS &sds)
{
    this->cat(sds.buf(), sds.len());
}

/*
void SDS::catrepr(const char* p, size_t len)
{

}
*/

void SDS::cpy(const char *t, size_t len)
{
    size_t totlen = sdshdr_->free + sdshdr_->len;

    if (totlen < len)
    {
        this->MakeRoomFor(len - sdshdr_->len);
        totlen = sdshdr_->free + sdshdr_->len;
    }

    memcpy(sdshdr_->buf, t, len);
    sdshdr_->buf[len] = '\0';
    sdshdr_->len = len;
    sdshdr_->free = totlen - len;
}

void SDS::cpy(const char *t)
{
    this->cpy(t, strlen(t));
}

void SDS::mapchars(const char *from, const char *to, size_t setlen)
{
    char *s = sdshdr_->buf;
    for (int i = 0; i < sdshdr_->len; ++i)
        for (int j = 0; j < setlen; ++j)
            if (s[i] == from[j])
            {
                s[i] = to[j];
                break;
            }
}

int SDS::cmp(const SDS &sds) const
{
    size_t l1, l2, minlen;
    int cmp;

    l1 = this->len();
    l2 = sds.len();
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(this->buf(), sds.buf(), minlen);

    if (cmp == 0)
        return l1 - l2;
    return cmp;
}

int SDS::cmp(const char *t) const
{
    size_t l1, l2, minlen;
    int cmp;

    l1 = this->len();
    l2 = strlen(t);
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(this->buf(), t, minlen);

    if (cmp == 0)
        return l1 - l2;
    return cmp;
}

void SDS::tolower()
{
    char *s = sdshdr_->buf;
    for (int i = 0; i < len(); ++i)
        s[i] = std::tolower(s[i]);
}

void SDS::toupper()
{
    char *s = sdshdr_->buf;
    for (int i = 0; i < len(); ++i)
        s[i] = std::toupper(s[i]);
}

void SDS::updatelen()
{
    int reallen = strlen(sdshdr_->buf);
    sdshdr_->free += (sdshdr_->len - reallen);
    sdshdr_->len = reallen;
}

void SDS::clear()
{
    sdshdr_->free += sdshdr_->len;
    sdshdr_->len = 0;
    sdshdr_->buf[0] = '\0';
}

void SDS::trim(const char *cset)
{
    char *start, *end, *sp, *ep;
    size_t len;

    sp = start = sdshdr_->buf;
    ep = end = sdshdr_->buf + this->len() - 1;

    while (sp <= end && strchr(cset, *sp))
        sp++;
    while (ep > start && strchr(cset, *ep))
        ep--;

    len = (sp > ep) ? 0 : (ep - sp + 1);
    if (sdshdr_->buf != sp)
        memmove(sdshdr_->buf, sp, len);
    sdshdr_->buf[len] = '\0';
    sdshdr_->free = sdshdr_->free + (sdshdr_->len - len);
    sdshdr_->len = len;
}

void SDS::range(int start, int end)
{
    SDSHDR *sh = sdshdr_;
    size_t newlen, len = this->len();

    if (len == 0) return;
    if (start < 0) {
        start = len+start;
        if (start < 0) start = 0;
    }
    if (end < 0) {
        end = len+end;
        if (end < 0) end = 0;
    }
    newlen = (start > end) ? 0 : (end-start)+1;
    if (newlen != 0) {
        if (start >= (signed)len) {
            newlen = 0;
        } else if (end >= (signed)len) {
            end = len-1;
            newlen = (start > end) ? 0 : (end-start)+1;
        }
    } else {
        start = 0;
    }
    if (start && newlen) memmove(sh->buf, sh->buf+start, newlen);
    sh->buf[newlen] = 0;
    sh->free = sh->free+(sh->len-newlen);
    sh->len = newlen;
    
}

void SDS::MakeRoomFor(size_t addlen)
{
    SDSHDR *newsh;
    size_t free = this->vail();
    size_t len = this->len();
    size_t newlen;

    if (free >= addlen)
        return;

    newlen = (len + addlen);
    if (newlen < SDS_MAX_PREALLOC)
        newlen *= 2;
    else
        newlen += SDS_MAX_PREALLOC;

    newsh = (SDSHDR *)realloc(sdshdr_, sizeof(SDSHDR) + newlen + 1);
    if (newsh == NULL)
        throw std::runtime_error("Failed to allocate memory");

    newsh->free = newlen - len;
    sdshdr_ = newsh;
}

void SDS::RemoveFreeSpace()
{
    sdshdr_ = (SDSHDR*)realloc(sdshdr_, sizeof(SDSHDR) + sdshdr_->len + 1);
    sdshdr_->free = 0;
}

void SDS::IncrLen(int incr)
{
    if(incr >= 0)
        assert(sdshdr_->free >= (unsigned int)incr);
    else
        assert(sdshdr_->free >= (unsigned int)(-incr));
    sdshdr_->len += incr;
    sdshdr_->free -= incr;
    sdshdr_->buf[sdshdr_->len] = '\0';
}

size_t SDS::AllocSize()
{
    return sizeof(*sdshdr_) + sdshdr_->len + sdshdr_->free + 1;
}

#ifdef SDS_TEST_MAIN
#include <iostream>
int main()
{
    {
        std::cout << "SDS::SDS(const void *init, size_t initlen)" << std::endl;
        SDS sds("hello", 1000);
        std::cout << sds << std::endl;
        std::cout << "len(): " << sds.len() << std::endl;
        std::cout << "vail(): " << sds.vail() << std::endl;
        std::cout << "buf(): " << sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS::SDS(const char *init)" << std::endl;
        SDS sds("hello");
        std::cout << sds << std::endl;
        std::cout << "len(): " << sds.len() << std::endl;
        std::cout << "vail(): " << sds.vail() << std::endl;
        std::cout << "buf(): " << sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS::SDS(void)" << std::endl;
        SDS sds;
        std::cout << sds << std::endl;
        std::cout << "len(): " << sds.len() << std::endl;
        std::cout << "vail(): " << sds.vail() << std::endl;
        std::cout << "buf(): " << sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS::SDS(long long value)" << std::endl;
        SDS sds(-129867LL);
        std::cout << sds << std::endl;
        std::cout << "len(): " << sds.len() << std::endl;
        std::cout << "vail(): " << sds.vail() << std::endl;
        std::cout << "buf(): " << sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS::SDS(unsigned long long value)" << std::endl;
        SDS sds(7298679LL);
        std::cout << sds << std::endl;
        std::cout << "len(): " << sds.len() << std::endl;
        std::cout << "vail(): " << sds.vail() << std::endl;
        std::cout << "buf(): " << sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS::SDS(const SDS &sds)" << std::endl;
        SDS sds(7298679LL);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        SDS sds2 = sds;
        std::cout << sds2 << std::endl;
        std::cout << "sds2 len(): " << sds2.len() << std::endl;
        std::cout << "sds2 vail(): " << sds2.vail() << std::endl;
        std::cout << "sds2 buf(): " << (void *)sds2.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS& SDS::operator=(const SDS &sds)" << std::endl;
        SDS sds(7298679LL);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        SDS sds2;
        sds2 = sds;
        std::cout << sds2 << std::endl;
        std::cout << "sds2 len(): " << sds2.len() << std::endl;
        std::cout << "sds2 vail(): " << sds2.vail() << std::endl;
        std::cout << "sds2 buf(): " << (void *)sds2.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS::SDS(SDS &&sds)" << std::endl;
        SDS sds(7298679LL);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        SDS sds2 = std::move(sds);
        std::cout << sds2 << std::endl;
        std::cout << "sds2 len(): " << sds2.len() << std::endl;
        std::cout << "sds2 vail(): " << sds2.vail() << std::endl;
        std::cout << "sds2 buf(): " << (void *)sds2.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS& SDS::operator=(SDS &&sds)" << std::endl;
        SDS sds(7298679LL);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        SDS sds2;
        sds2 = std::move(sds);
        std::cout << sds2 << std::endl;
        std::cout << "sds2 len(): " << sds2.len() << std::endl;
        std::cout << "sds2 vail(): " << sds2.vail() << std::endl;
        std::cout << "sds2 buf(): " << (void *)sds2.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "SDS *SDS::splitlen(const char *s, int len, const char *sep, int seplen, int &count)" << std::endl;
        const char *str = "wcd--cde--fc--fgb--g++--ccd--wwd----wwd";
        auto token = SDS::splitlen(str, strlen(str), "--", 2);

        for (int i = 0; i < token.size(); ++i)
        {
            std::cout << token[i] << std::endl;
            std::cout << "sds len(): " << token[i].len() << std::endl;
            std::cout << "sds vail(): " << token[i].vail() << std::endl;
            std::cout << "sds buf(): " << (void *)token[i].buf() << std::endl;
        }
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::growzero(size_t len)" << std::endl;
        SDS sds("hello world");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.growzero(30);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.growzero(31);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::cat(const void *t, size_t len)" << std::endl;
        SDS sds("hello world");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cat("1234", 2);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cat("1234", 4);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::cat(const char *t)" << std::endl;
        SDS sds("hello world");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cat("1234");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cat("5678", 4);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::cat(const SDS &sds)" << std::endl;
        SDS sds("hello world");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cat(SDS("1234"));
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cat(SDS("5678"));
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::cpy(const char* t, size_t len)" << std::endl;
        SDS sds("hello world");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cpy("123", 3);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cpy("adsfkjhadksfhaksdjfhakjsdfhakjsdhfaksdjhfasd", 30);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::cpy(const char* t)" << std::endl;
        SDS sds("hello world");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cpy("1234");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.cpy("hello world, you");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::mapchars(const char* from, const char* to, size_t setlen)" << std::endl;
        SDS sds("hello world");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        sds.mapchars("hewo", "cate", 4);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "int SDS::cmp(const SDS& sds) const" << std::endl;
        SDS sds("hello");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        std::cout << "sds.cmp(SDS(\"hello\")): " << sds.cmp(SDS("hello")) << std::endl;
        std::cout << "sds.cmp(SDS(\"hello1\")): " << sds.cmp(SDS("hello1")) << std::endl;
        std::cout << "sds.cmp(SDS(\"hell\")): " << sds.cmp(SDS("hell")) << std::endl;
        std::cout << "sds.cmp(SDS(\"hallo\")): " << sds.cmp(SDS("hallo")) << std::endl;
        std::cout << "sds.cmp(SDS(\"hfllo\")): " << sds.cmp(SDS("hfllo")) << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "int SDS::cmp(const char* t) const" << std::endl;
        SDS sds("hello");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;

        std::cout << "sds.cmp(\"hello\"): " << sds.cmp("hello") << std::endl;
        std::cout << "sds.cmp(\"hello1\"): " << sds.cmp("hello1") << std::endl;
        std::cout << "sds.cmp(\"hell\"): " << sds.cmp("hell") << std::endl;
        std::cout << "sds.cmp(\"hallo\"): " << sds.cmp("hallo") << std::endl;
        std::cout << "sds.cmp(\"hfllo\"): " << sds.cmp("hfllo") << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "int SDS::tolower()" << std::endl;
        SDS sds("heLLo");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
        sds.tolower();
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "int SDS::toupper()" << std::endl;
        SDS sds("heLLo");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
        sds.toupper();
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "int SDS::updatelen()" << std::endl;
        SDS sds("heLLo");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
        sds.buf()[2] = '@';
        sds.buf()[3] = '\0';
        sds.updatelen();
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "int SDS::clear()" << std::endl;
        SDS sds("heLLo");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
        sds.clear();
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::trim(const char *cset)" << std::endl;
        SDS sds("heL:Lo-H,*");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
        sds.trim("*H-,");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }
    std::cout << std::endl;
    {
        std::cout << "void SDS::range(int start, int end)" << std::endl;
        SDS sds("hello");
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
        sds.range(1, -1);
        std::cout << sds << std::endl;
        std::cout << "sds len(): " << sds.len() << std::endl;
        std::cout << "sds vail(): " << sds.vail() << std::endl;
        std::cout << "sds buf(): " << (void *)sds.buf() << std::endl;
    }

    return 0;
}

#endif
