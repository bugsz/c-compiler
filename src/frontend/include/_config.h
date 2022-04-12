#if COLOR_TERM == 1
#define COLOR_RED "\033[1;31m"
#define COLOR_NORMAL "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_GREEN "\033[1;32m"
#else
#define COLOR_RED ""
#define COLOR_NORMAL ""
#define COLOR_BOLD ""
#define COLOR_GREEN ""
#endif

#if defined __GNUG__ || defined __clang__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif