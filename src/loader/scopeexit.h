#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_SCOPEEXIT_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_SCOPEEXIT_H

template <typename Function> class ScopeExit {
    Function func;

  public:
    explicit ScopeExit(Function func) : func(func) {}
    ~ScopeExit() { func(); }

    // Prevent copying
    ScopeExit(const ScopeExit &) = delete;
    ScopeExit &operator=(const ScopeExit &) = delete;
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_SCOPEEXIT_H
