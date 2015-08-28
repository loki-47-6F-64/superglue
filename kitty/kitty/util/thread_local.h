#ifndef KITTY_UTIL_THREAD_LOCAL_H
#define KITTY_UTIL_THREAD_LOCAL_H

#include <type_traits>
#include <mutex>

#ifndef LACKS_FEATURE_THREAD_LOCAL
#define THREAD_LOCAL thread_local

namespace util {
template<class T>
class ThreadLocal {
  template<class Z, class X = void>
  struct helper_type;
    
  template<class Z>
  struct helper_type<Z, typename std::enable_if<std::is_array<Z>::value>::type> {
    typedef Z class_t;
    typedef typename std::remove_all_extents<class_t>::type default_type;
  };

  template<class Z>
  struct helper_type<Z, typename std::enable_if<std::is_copy_constructible<Z>::value>::type> {
    typedef Z class_t;
    typedef class_t default_type;
  };

  typedef typename helper_type<T>::class_t class_t;
  typedef typename helper_type<T>::default_type default_type;

  class_t _default;
public:
  ThreadLocal() = default;
  ThreadLocal(const default_type &t) : _default { t } {}
  ThreadLocal(default_type &&t) : _default { std::move(t) } {}

  operator class_t & () { return get(); }
    
  class_t &get() {
    return _default;
  }

  class_t &operator = (class_t &&val) {
    get() = std::move(val);
    
    return get();
  }
  
  class_t &operator = (const class_t &val) {
    get() = val;
    
    return get();
  }
};
}
#else

#include <pthread.h>

#define THREAD_LOCAL
namespace util {
template<class T>
class ThreadLocal {
  template<class Z, class X = void>
  struct helper_type;
  
  template<class Z>
  struct helper_type<Z, typename std::enable_if<std::is_array<Z>::value>::type> {
    static void destroy(void *ptr) {
      delete[] reinterpret_cast<T*>(ptr);
    }
      
    typedef Z class_t;
    typedef typename std::remove_all_extents<class_t>::type primitive;
    typedef primitive *pointer;
    typedef primitive default_type;
  };

  template<class Z>
  struct helper_type<Z, typename std::enable_if<std::is_copy_constructible<Z>::value>::type> {
    static void destroy(void *ptr) {
      delete reinterpret_cast<T*>(ptr);
    }
      
    typedef Z class_t;
    
    typedef class_t primitive;
    typedef primitive *pointer;
    
    typedef class_t default_type;
  };
  
  typedef typename helper_type<T>::class_t class_t;
  typedef typename helper_type<T>::primitive primitive;
  typedef typename helper_type<T>::pointer pointer;
  typedef typename helper_type<T>::default_type default_type;

  pthread_key_t &key() {
    static pthread_key_t k;
    
    return k;
  }

  default_type _default;
public:
  ThreadLocal() { ThreadLocal(default_type()); }
  ThreadLocal(const default_type &t) : _default { t } {
    static std::once_flag once;
    
    std::call_once(once, [&]() {
      pthread_key_create(&key(), &helper_type<T>::destroy);
    });
  }

  ThreadLocal(default_type &&t) : _default { std::move(t) } {
    static std::once_flag once;
    
    std::call_once(once, [&]() {
      pthread_key_create(&key(), &helper_type<T>::destroy);
    });
  }

  ThreadLocal(const ThreadLocal &) = delete;
  ThreadLocal(ThreadLocal &&) = delete;

  operator class_t & () { return get(); }
    
  class_t &get() {
    pointer val = reinterpret_cast<pointer>(pthread_getspecific(key()));
      
    if(!val) pthread_setspecific(key(), val = new class_t { _default });
      
    return *reinterpret_cast<class_t*>(val);
  }
  
  class_t &operator = (class_t &&val) {
    get() = std::move(val);
    
    return get();
  }
  
  class_t &operator = (const class_t &val) {
    get() = val;
    
    return get();
  }
};
}
#endif
#endif
