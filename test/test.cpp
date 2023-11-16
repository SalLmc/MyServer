#include "../src/buffer/buffer.h"
#include <assert.h>
#include <list>
#include <stdio.h>

int main()
{
    std::list<int> ls = {1, 2, 3};
    auto x = ls.begin();
    printf("%d\n", *x);
    x++;
    printf("%d\n", *x);
    ls.insert(x, 5);

    printf("<<<<<<<<<<<<<<<<<<\n");
    for (auto x : ls)
    {
        printf("%d\n", x);
    }

    printf("<<<<<<<<<<<<<<<<<<\n");
    std::list<int> ls1;
    assert(ls1.begin() == ls1.end());

    ls1.insert(ls1.end(), 1);
    for (auto x : ls1)
    {
        printf("%d\n", x);
    }

    printf("<<<<<<<<<<<<<<<<<<\n");
    std::list<int> ls2 = {2};
    auto iter = ls2.insert(ls2.begin(), 1);
    printf("%d\n", *iter);
    iter++;
    printf("%d\n", *iter);

    printf("<<<<<<<<<<<<<<<<<<\n");
    LinkedBuffer buffer;
    buffer.insertNewNode(buffer.nodes_.end());
    buffer.append("123456789");
    auto beginPtr = &buffer.nodes_.front();
    auto thisIter = buffer.nodes_.begin();
    thisIter++;
    buffer.insertNewNode(thisIter);
    beginPtr->next_->append("xyz", 3);

    while (beginPtr != NULL)
    {
        printf("%s\n", beginPtr->toString().c_str());
        beginPtr = beginPtr->next_;
    }

    for (auto &x : buffer.memoryMap)
    {
        printf("%ld, %ld\n", x.first, (uint64_t)x.second->start_);
    }
}