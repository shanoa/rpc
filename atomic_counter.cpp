#include "atomic_counter.h"
#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

atomic_counter::atomic_counter()
    : count(1)
{
}

atomic_counter::~atomic_counter()
{
}

long atomic_counter::inc()
{
#ifdef __GNUC__
    return __sync_fetch_and_add(&count, 1);
#else
#ifdef _WIN32
    return InterlockedIncrement(&count);
#endif /* _WIN32 */
#endif /* __GUNC__ */
}

long atomic_counter::dec()
{
#ifdef __GNUC__
    return __sync_fetch_and_sub(&count, 1);
#else
#ifdef _WIN32
    return InterlockedDecrement(&count);
#endif /* _WIN32 */
#endif /* __GUNC__ */
}
