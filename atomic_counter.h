#ifndef ATOMIC_COUNTER_H_INCLUDED
#define ATOMIC_COUNTER_H_INCLUDED

class atomic_counter {
public:
    atomic_counter();
    ~atomic_counter();

    long inc();
    long dec();

private:
    volatile long count;
};

#endif /* ATOMIC_COUNTER_H_INCLUDED */
