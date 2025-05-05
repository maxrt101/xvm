#ifndef _XVM_STACK_H_
#define _XVM_STACK_H_ 1

#include <cstddef>

template <typename T, int N = 256>
class Stack {
 private:
  T m_stack[N];
  T* m_stackTop = nullptr;

 public:
  inline Stack() {
    reset();
  }

  inline ~Stack() {}

  inline void reset() {
    m_stackTop = m_stack;
  }

  inline size_t size() const {
    return m_stackTop-m_stack;
  }

  inline void push(T value) {
    // if (m_stackTop > m_stack + N) {}
    *m_stackTop++ = value;
  }

  inline T pop() {
    // if (m_stackTop <= m_stack) {}
    return *(--m_stackTop);
  }

  inline T peek(int distance = 0) const {
    return m_stackTop[-1-distance];
  }
};

#endif