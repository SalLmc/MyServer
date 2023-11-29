#include "../src/headers.h"

#include "../src/buffer/buffer.h"

int main()
{
    LinkedBuffer buffer;
    buffer.append("123456789");

    for (auto &x : buffer.memoryMap_)
    {
        printf("%ld, %ld\n", x.first, (uint64_t)x.second->start_);
    }

    LinkedBufferNode *ptr;
    LinkedBufferNode *p = &buffer.nodes_.front();
    ptr = p->next_;

    ptr->pos_ = 1;
    uint64_t addr = (uint64_t)(ptr->start_ + ptr->pos_);
    printf("%ld\n", addr & (~(LinkedBufferNode::NODE_SIZE - 1)));

    // '6'
    printf("%c\n", *((char *)addr));

    // '1'
    auto x = buffer.getNodeByAddr(addr);
    printf("%ld\n", x->pos_);

    // '6'
    printf("%c\n", *(x->start_ + x->pos_));

    ptr = ptr->next_;
    addr = (uint64_t)(ptr->start_ + ptr->pos_);

    // '9'
    printf("%c\n", *((char *)addr));

    x = buffer.getNodeByAddr(addr);

    // '9'
    printf("%c\n", *(x->start_ + x->pos_));

    auto iter = buffer.nodes_.begin();
    printf("%s\n", iter->toString().c_str());
    buffer.insertNewNode(iter);
    printf("%s\n", iter->toString().c_str());
    iter++;

    ptr = p->next_;

    printf("%ld, %ld\n", (uint64_t)ptr, (uint64_t)iter->start_);

    iter->append("abc", 3);
}