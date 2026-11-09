#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_SCOPEEXIT_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_SCOPEEXIT_H

template <typename Function> class ScopeExit {
    Function SavedFunc;

  public:
    explicit ScopeExit(Function Func) : SavedFunc(Func) {}
    ~ScopeExit() { SavedFunc(); }

    // Prevent copying
    ScopeExit(const ScopeExit &) = delete;
    ScopeExit &operator=(const ScopeExit &) = delete;
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_SCOPEEXIT_H
