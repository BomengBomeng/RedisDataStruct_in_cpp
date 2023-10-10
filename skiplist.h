#ifndef BOMENG_REDIS_SKUPLIST_H
#define BOMENG_REDIS_SKIPLIST_H

#include <vector>
#include <cstddef>
#include <cassert>
#include <ostream>
#include <functional>

template <typename K, typename V, typename Cmp = std::less<K>>
class SKIPLIST
{
private:
    struct skiplistNode
    {
        struct skiplistLevel
        {
            skiplistNode *forward = nullptr;
            unsigned int span;
        };

        std::pair<K, V> data;
        skiplistNode *backward = nullptr;
        std::vector<skiplistLevel> level;

        skiplistNode(K s, V val, size_t l) : data({s, val}), level(l)
        {
            assert(l >= 1);
        }
    };

    static constexpr int maxLevel = 8;
    static constexpr double probability = 0.75;

public:
    class iterator : public std::iterator<std::bidirectional_iterator_tag, void *>
    {
    private:
        skiplistNode *node_;

    public:
        iterator(skiplistNode *node) : node_(node) {}
        iterator &operator++()
        {
            node_ = node_->level[0].forward();
            return *this;
        }
        iterator operator++(int)
        {
            iterator it(node_);
            node_ = node_->level[0].forward();
            return it;
        }
        iterator &operator--()
        {
            node_ = node_->backward;
            return *this;
        }
        iterator operator--(int)
        {
            iterator it(node_);
            node_ = node_->backward;
            return it;
        }

        bool operator==(const iterator &other) const { return node_ == other.node_; }
        bool operator!=(const iterator &other) const { return node_ != other.node_; }
        std::pair<K, V> &operator*() const { return node_->data; }
        std::pair<K, V> *operator->() const { return &(node_->data); }
    };

    class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const void *>
    {
    private:
        const skiplistNode *node_;

    public:
        const_iterator(const skiplistNode *node) : node_(node) {}
        const_iterator &operator++()
        {
            node_ = node_->level[0].forward();
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator it(node_);
            node_ = node_->level[0].forward();
            return it;
        }
        const_iterator &operator--()
        {
            node_ = node_->backward;
            return *this;
        }
        const_iterator operator--(int)
        {
            const_iterator it(node_);
            node_ = node_->backward;
            return it;
        }

        bool operator==(const const_iterator &other) const { return node_ == other.node_; }
        bool operator!=(const const_iterator &other) const { return node_ != other.node_; }
        const std::pair<K, V> &operator*() const { return node_->data; }
        const std::pair<K, V> *operator->() const { return &(node_->data); }
    };

private:
    skiplistNode *header_;
    skiplistNode *tail_;
    size_t length_;
    size_t level_;

private:
    size_t randomLevel()
    {
        size_t level = 1;
        while ((random() & 0xFFFF) < (probability * 0xFFFF))
            level += 1;
        return (level < maxLevel) ? level : maxLevel;
    }

    skiplistNode *_find(K key) const
    {
        skiplistNode *x = header_;
        for (int i = level_ - 1; i >= 0; --i)
            while (x->level[i].forward && Cmp()(x->level[i].forward->data.first, key))
                x = x->level[i].forward;

        if (x->level[0].forward && !Cmp()(key, x->level[0].forward->data.first))
            return x->level[0].forward;

        return nullptr;
    }

public:
    SKIPLIST()
    {
        srand(time(NULL));
        level_ = 1;
        length_ = 0;
        header_ = new skiplistNode(K(), V(), maxLevel);
        for (int i = 0; i < maxLevel; ++i)
        {
            header_->level[i].forward = nullptr;
            header_->level[i].span = 0;
        }
        header_->backward = nullptr;
        tail_ = header_;
    }

    SKIPLIST(const SKIPLIST &sl)
    {
        // 初始化
        level_ = 1;
        length_ = 0;
        header_ = new skiplistNode(K(), V(), maxLevel);
        for (int i = 0; i < maxLevel; ++i)
        {
            header_->level[i].forward = nullptr;
            header_->level[i].span = 0;
        }
        header_->backward = nullptr;
        tail_ = header_;

        // 拷贝
        std::vector<skiplistNode*> vec;
        vec.reserve(sl.length_+1);
        vec.push_back(header_);
        skiplistNode* x = sl.header_->level[0].forward;
        while(x){
            tail_->level[0].forward = new skiplistNode(x->data.first, x->data.second, x->level.size());
            tail_->level[0].forward->backward = tail_;
            tail_ = tail_->level[0].forward;
            vec.push_back(tail_);

            x = x->level[0].forward;
        }

        // 修改level数组
        int idx = 0;
        x = sl.header_;
        while (x->level[0].forward)
        {
            for(int i=0; i<vec[idx]->level.size(); ++i)
            {
                vec[idx]->level[i].span = x->level[i].span;
                vec[idx]->level[i].forward = vec[idx  +  x->level[i].span];
            }
            x = x->level[0].forward;
            idx++;
        }
        for(int i=0; i<tail_->level.size(); ++i)
        {
            tail_->level[i].span = 0;
            tail_->level[i].forward = nullptr;
        }
    }

    SKIPLIST &operator=(const SKIPLIST &sl)
    {
        if(this == &sl)
            return *this;
            
        SKIPLIST sl2(sl);
        *this = std::move(sl2);
        return *this;
    }

    SKIPLIST(SKIPLIST &&sl)
    {
        header_ = sl.header_;
        tail_ = sl.tail_;
        length_ = sl.length_;
        level_ = sl.level_;

        sl.level_ = 1;
        sl.length_ = 0;
        sl.header_ = new skiplistNode(K(), V(), maxLevel);
        for (int i = 0; i < maxLevel; ++i)
        {
            sl.header_->level[i].forward = nullptr;
            sl.header_->level[i].span = 0;
        }
        sl.header_->backward = nullptr;
        sl.tail_ = sl.header_;
    }

    SKIPLIST &operator=(SKIPLIST &&sl)
    {
        if (this = &sl)
            return *this;

        this->clear();
        skiplistNode *t = header_;
        header_ = sl.header_;
        tail_ = sl.tail_;
        length_ = sl.length_;
        level_ = sl.level_;
        sl.header_ = t;
        sl.tail_ = sl.header_;
        sl.length_ = 0;
        sl.level_ = 1;

        return *this;
    }

    ~SKIPLIST()
    {
        clear();
        delete header_;
        header_ = nullptr;
        tail_ = nullptr;
    }

public:
    iterator begin() { return iterator(header_->level[0]); }
    iterator end() { return iterator(nullptr); }
    const_iterator cbegin() { return const_iterator(header_->level[0]); }
    const_iterator cend() { return const_iterator(nullptr); }
    size_t size() { return length_; }
    bool empty() { return length_ == 0; }

public:
    bool insert(K key, V val)
    {
        if (find(key) != end())
            return false;

        skiplistNode *update[maxLevel];
        size_t rank[maxLevel];
        skiplistNode *x = header_;

        for (int i = level_ - 1; i >= 0; --i)
        {
            rank[i] = i == (level_ - 1) ? 0 : rank[i + 1];
            while (x->level[i].forward && (Cmp()(x->level[i].forward->data.first, key)))
            {
                rank[i] += x->level[i].span;
                x = x->level[i].forward;
            }
            update[i] = x;
        }

        int level = randomLevel();
        if (level > level_)
        {
            for (int i = level_; i < level; ++i)
            {
                rank[i] = 0;
                update[i] = header_;
                update[i]->level[i].span = length_;
            }
            level_ = level;
        }

        x = new skiplistNode(key, val, level);
        for (int i = 0; i < level; ++i)
        {
            x->level[i].forward = update[i]->level[i].forward;
            update[i]->level[i].forward = x;
            x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
            update[i]->level[i].span = (rank[0] - rank[i]) + 1;
        }

        for (int i = level; i < level_; ++i)
            update[i]->level[i].span++;

        x->backward = update[0]; //(update[0] == header_)? nullptr : update[0];
        if (x->level[0].forward)
            x->level[0].forward->backward = x;
        else
            tail_ = x;

        ++length_;

        return true;
    }

    bool erase(iterator it)
    {
        if(it == end())
            return false;
        
        return erase(it->first);
    }

    bool erase(K key)
    {
        skiplistNode *update[maxLevel];
        skiplistNode *x = header_;

        for(int i=level_-1; i>=0; --i)
        {
            while (x->level[i].forward && Cmp()(x->level[i].forward->data.first, key))
                x = x->level[i].forward;
            update[i] = x;
        }

        x = x->level[0].forward;
        if(!x || Cmp()(x->data.first, key))
            return false;
        
        for(int i=0; i<level_; ++i)
        {
            if(update[i]->level[i].forward == x)
            {
                update[i]->level[i].span += x->level[i].span - 1;
                update[i]->level[i].forward = x->level[i].forward;
            }
            else
                update[i]->level[i].span -= 1;
        }

        if(x->level[0].forward)
            x->level[0].forward->backward = x->backward;
        else
            tail_ = x->backward;

        while(level_ > 1 && header_->level[level_-1].forward == nullptr)
            --level_;
        
        delete x;
        --length_;

        return true;
    }

    void clear()
    {
        skiplistNode *t = header_->level[0].forward;
        while (t != nullptr)
        {
            skiplistNode *next = t->level[0].forward;
            delete t;
            t = next;
        }
        for (int i = 0; i < maxLevel; ++i)
        {
            header_->level[i].forward = nullptr;
            header_->level[i].span = 0;
        }
        level_ = 1;
        length_ = 0;
        header_->backward = nullptr;
        tail_ = header_;
    }

    iterator find(K key)
    {
        return iterator(_find(key));
    }

    const_iterator find(K key) const
    {
        return const_iterator(_find(key));
    }

    // for test
    template <typename K_, typename V_, typename Cmp_>
    friend std::ostream &operator<<(std::ostream &os, const SKIPLIST<K_, V_, Cmp_> &sl);
};

#endif