#ifndef BOMENG_REDIS_INTSET_H
#define BOMENG_REDIS_INTSET_H

#include <stdint.h>
#include <tuple>
#include <cstddef>

namespace bRedis
{

    class INTSET
    {
    public:
        struct intset
        {
            uint32_t encoding;
            uint32_t length;
            int8_t contents[];
        };

    private:
        intset *is_;

    private:
        void resize(uint32_t len);
        std::tuple<bool, uint32_t> search(int64_t value) const;
        void upgradeAndAdd(int64_t value);
        void moveTail(uint32_t from, uint32_t to);

    public:
        INTSET();

        INTSET(const INTSET &in);
        INTSET &operator=(const INTSET &in);
        INTSET(INTSET &&in);
        INTSET &operator=(INTSET &&in);

        ~INTSET();

    public:
        bool add(int64_t value);
        bool remove(int64_t value);
        bool find(int64_t value) const;
        int64_t random() const;
        std::tuple<bool, int64_t> get(uint32_t pos) const;

    public:
        uint32_t len() const;
        uint32_t encoding() const;
        size_t bloLen() const;
    };

} // namespace bRedis

#endif