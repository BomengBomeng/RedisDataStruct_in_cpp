#include "skiplist.h"
#include <iostream>
#include <string>
#include <random>
#include <algorithm>
#include <vector>
#include <iomanip>

#define ptrp(ptr) std::showbase << std::internal << std::setfill('0') << std::setw(14) << std::hex << reinterpret_cast<uintptr_t>(ptr)
#define valp(val) std::setfill(' ') << std::setw(1) << std::dec << (val)

template<typename K, typename V, typename Cmp>
std::ostream& operator<<(std::ostream& os, const SKIPLIST<K, V, Cmp> &sl)
{
    auto *t = sl.header_;
    while (t)
    {
        os << ptrp(t) << " "  << " data(" << valp(t->data.first) << ") " << ptrp(t->backward) << "  |  ";
        // os << t->level.size() << " ";
        for(int i=0; i<t->level.size(); ++i)
            os << valp(t->level[i].span) << "_" << ptrp(t->level[i].forward) << " ";
        os << "\n";

        t = t->level[0].forward;
    }
    return os;
}

std::vector<size_t> random_shuffle(size_t n) {
    // 创建一个伪随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());

    // 创建一个包含 1~n 的向量
    std::vector<size_t> v(n);
    std::iota(v.begin(), v.end(), 1);

    // 使用 std::shuffle 算法随机排序
    std::shuffle(v.begin(), v.end(), gen);

    // 返回随机排序后的向量
    return v;
}

int main()
{
    std::cout << "less: " << std::endl; {
        SKIPLIST<int, std::string, std::less<int>> skiplist;
        std::vector<size_t> nums = random_shuffle(9);
        for (auto num : nums)
            skiplist.insert(num, "zhang");
        std::cout << skiplist << std::endl;
    }

    std::cout << "greater: " << std::endl; {
        SKIPLIST<int, std::string, std::greater<int>> skiplist;
        std::vector<size_t> nums = random_shuffle(9);
        for (auto num : nums)
            skiplist.insert(num, "zhang");
        std::cout << skiplist << std::endl;
    }

    std::cout << "less find: " << std::endl; {
        SKIPLIST<int, std::string, std::less<int>> skiplist;
        const size_t count = 9;
        std::vector<size_t> nums = random_shuffle(count);
        for (auto num : nums)
        {
            skiplist.insert(num, "zhang");
            std::cout << num << " ";
        }
        std::cout << "\n";

        for(int i=1; i<=count; ++i)
            std::cout << "find " << i << ": " << (bool(skiplist.find(i)!=skiplist.end())? "true" : "false") << std::endl;
        for(int i=count; i<=2*count; ++i)
            std::cout << "find " << i << ": " << (bool(skiplist.find(i)!=skiplist.end())? "true" : "false") << std::endl;
    }

    std::cout << "\ngreater find: " << std::endl; {
        SKIPLIST<int, std::string, std::greater<int>> skiplist;
        const size_t count = 9;
        std::vector<size_t> nums = random_shuffle(count);
        for (auto num : nums)
        {
            skiplist.insert(num, "zhang");
            std::cout << num << " ";
        }
        std::cout << "\n";

        for(int i=1; i<=count; ++i)
            std::cout << "find " << i << ": " << (bool(skiplist.find(i)!=skiplist.end())? "true" : "false") << std::endl;
        for(int i=count; i<=2*count; ++i)
            std::cout << "find " << i << ": " << (bool(skiplist.find(i)!=skiplist.end())? "true" : "false") << std::endl;
    }

    std::cout << "\nclear: " << std::endl; {
        SKIPLIST<int, std::string, std::greater<int>> skiplist;
        const size_t count = 9;
        std::vector<size_t> nums = random_shuffle(count);
        for (auto num : nums)
        {
            skiplist.insert(num, "zhang");
        }

        std::cout << skiplist << std::endl;

        skiplist.clear();

        std::cout << skiplist << std::endl;
    }

    std::cout << "\nerase: " << std::endl; {
        SKIPLIST<int, std::string, std::greater<int>> skiplist;
        const size_t count = 9;
        std::vector<size_t> nums = random_shuffle(count);
        for (auto num : nums)
        {
            skiplist.insert(num, "zhang");
        }

        std::cout << skiplist << std::endl;

        for(int i=3; i<=5; ++i)
            skiplist.erase(i);
        std::cout << skiplist << std::endl;

        skiplist.erase(4);
        std::cout << skiplist << std::endl;
    }

    std::cout << "\nerase iterator: " << std::endl; {
        SKIPLIST<int, std::string, std::greater<int>> skiplist;
        const size_t count = 9;
        std::vector<size_t> nums = random_shuffle(count);
        for (auto num : nums)
        {
            skiplist.insert(num, "zhang");
        }

        std::cout << skiplist << std::endl;

        for(int i=3; i<=5; ++i)
            skiplist.erase(skiplist.find(i));
        std::cout << skiplist << std::endl;
    }

    std::cout << "\ncopy move: " << std::endl; {
        SKIPLIST<int, std::string, std::greater<int>> sl1;
        const size_t count = 9;
        std::vector<size_t> nums = random_shuffle(count);
        for (auto num : nums)
        {
            sl1.insert(num, "zhang");
        }

        SKIPLIST<int, std::string, std::greater<int>> sl2 = std::move(sl1);
        std::cout << sl2 << std::endl;
        SKIPLIST<int, std::string, std::greater<int>> sl3(std::move(sl2));
        std::cout << sl3 << std::endl;

        SKIPLIST<int, std::string, std::greater<int>> sl4(sl3);
        std::cout << sl4 << std::endl;
        sl4.insert(10, "yang");
        // sl4.insert(-1, "liu");
        std::cout << sl4 << std::endl;

        SKIPLIST<int, std::string, std::greater<int>> sl5 = sl4;
        std::cout << sl5 << std::endl;
    }

    return 0;
}
