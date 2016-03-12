#ifndef SYNTAXIC_CORE_HOOKS_HPP
#define SYNTAXIC_CORE_HOOKS_HPP

#include <functional>
#include <map>
#include <memory>

template <typename ... Args>
class HookSource;

template <typename ... Args>
class Hook {
private:
  int id;
  std::shared_ptr<HookSource<Args...>*> hook_source;
public:
  Hook() : id(-1), hook_source(nullptr) {}
  Hook(int i, std::shared_ptr<HookSource<Args...>*> p) : id(i), hook_source(p) {}

  /** Can this hook ever be possibly invoked? */
  bool valid() {
    if (id >= 0 && hook_source) {
      HookSource<Args...>* ptr = *hook_source;
      if (ptr != nullptr) return true;
    }
    return false;
  }

  // Copy
  Hook(const Hook&) = delete;
  const Hook& operator=(const Hook&) = delete;
  
  // Move
  Hook(const Hook&&) = delete;
  Hook(Hook&& other) {
    if (valid()) (*hook_source)->delete_hook(id);
    id = other.id;
    hook_source = other.hook_source;

    other.id = -1;
    other.hook_source = nullptr;
  }
  const Hook& operator=(const Hook&&) = delete;
  Hook& operator=(Hook&& other) {
    if (valid()) (*hook_source)->delete_hook(id);
    id = other.id;
    hook_source = other.hook_source;

    other.id = -1;
    other.hook_source = nullptr;
    return *this;
  }
  
  ~Hook() {
    if (valid()) (*hook_source)->delete_hook(id);
  }
};

template <typename ... Args>
class HookSource {
  typedef std::function<void (Args...)> Callback;

private:
  int counter;

  std::shared_ptr<HookSource<Args ...>*> marker;

  std::map<int, Callback> callbacks;

public:
  HookSource() : counter(0), marker(std::make_shared<HookSource<Args ...>*>(this)) {}

  Hook<Args...> add(Callback callback) {
    counter++;
    callbacks[counter] = callback;
    return Hook<Args...>(counter, marker);
  }
  
  void call(Args ... args) {
    for (auto& p : callbacks) {
      p.second(args ...);
    }
  }
  
  void delete_hook(int id) {
    callbacks.erase(id);
  }
  
  int num_hooks() {
    return callbacks.size();
  }
  
  ~HookSource() {
    *marker = nullptr;
  }
};


#endif
