#pragma once

#if defined(NDEBUG)/*  || defined(__OPTIMIZE__) */
#    define MA_DEBUG 0
#else
#    define MA_DEBUG 1
#endif

#pragma region defer

namespace ma{
template<typename T>
struct exit_scope
{
    T lambda;
    exit_scope(T lambda):lambda(lambda){}
    ~exit_scope(){lambda();}
    exit_scope(const exit_scope&) = delete;
private:
    exit_scope& operator =(const exit_scope&) = delete;
};

class exit_scope_help
{
public:
    template<typename T>
    exit_scope<T> operator +(T t){return t;}
};
} // namespace ma

/// Concatenate preprocessor tokens a and b after macro-expanding them.
#define MA_CONCAT__(a, b) a ## b
#define MA_CONCAT_(a, b) MA_CONCAT__(a, b)
#define MA_CONCAT(a, b) MA_CONCAT_(a, b)
#define defer const auto& MA_CONCAT(_defer__, __LINE__) = ::ma::exit_scope_help() + [&]()

#pragma endregion defer

#define eprintf(_format, ...) fprintf(stderr, _format, ##__VA_ARGS__)

#if MA_DEBUG
#define dprintf(_format, ...) fprintf(stderr, _format, ##__VA_ARGS__)
#else
#define dprintf(_format, ...)
#endif

// #if !defined(max)
// #define max(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
// #endif