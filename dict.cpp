#include "dict.h"

class Entry : public DICTentry<int, int>{
public:
    unsigned int hashFunction() override {
        return 0;
    }

    Entry()
        : DICTentry(0, 0)
    {}
};

int main(){

    Entry e;

    DICT<Entry> dic;

    return 0;
}