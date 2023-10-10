#ifndef BOMENG_REDIS_SDS_H
#define BOMENG_REDIS_SDS_H

#include <sys/types.h>
#include <stdarg.h>
#include <ostream>
#include <vector>

#define SDS_MAX_PREALLOC (1024 * 1024)

namespace bRedis
{

    class SDS
    {
    private:
        struct SDSHDR
        {
            unsigned int len;
            unsigned int free;
            char buf[];
        };

    private:
        SDSHDR *sdshdr_;

    public:
        SDS(const void *init, size_t initlen);
        SDS(const char *init);
        SDS(void);
        SDS(long long value);
        SDS(unsigned long long value);

        SDS(const SDS &sds);
        SDS &operator=(const SDS &sds);
        SDS(SDS &&sds);
        SDS &operator=(SDS &&sds);

        ~SDS(void);

        // static std::tuple<SDS *, int> splitlen(const char *s, int len, const char *sep, int seplen);
        // static std::tuple<SDS *, int> splitargs(const char *line);
        static std::vector<SDS> splitlen(const char *s, int len, const char *sep, int seplen);
        static std::vector<SDS> splitargs(const char *line);
        static SDS join(char **argv, int argc, char *sep);

    public:
        inline size_t len() const { return sdshdr_->len; }
        inline size_t vail() const { return sdshdr_->free; }
        inline char *buf() { return sdshdr_->buf; }
        inline const char *buf() const { return sdshdr_->buf; }

    public:
        void growzero(size_t len);
        void cat(const void *t, size_t len);
        void cat(const char *t);
        void cat(const SDS &sds);
        void catrepr(const char *p, size_t len);
        void cpy(const char *t, size_t len);
        void cpy(const char *t);
        void mapchars(const char *from, const char *to, size_t setlen);

    public:
        int cmp(const SDS &sds) const;
        int cmp(const char *t) const;

    public:
        // bool operator==();

    public:
        void tolower();
        void toupper();

    public:
        void updatelen();
        void clear();
        void trim(const char *cset);
        void range(int start, int end);

    public:
        /* Low level functions exposed to the user API */
        void MakeRoomFor(size_t addlen);
        void RemoveFreeSpace();
        void IncrLen(int incr);
        size_t AllocSize();
    };

    std::ostream &operator<<(std::ostream &os, const SDS &sds)
    {
        os << sds.buf();
        return os;
    }

} // namespace bRedis

#endif