#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <utility>
#include <cstdint>

namespace util {
template<class T>
class Optional {
  class object {
  public:
    typedef T obj_t;
  private:
    
    uint8_t _obj[sizeof(obj_t)];
    obj_t *_obj_p;
    
  public:
    object() : _obj_p(nullptr) {}
    explicit object(obj_t &&obj) {
      // Call constructor at the address of _obj
      _obj_p = new (_obj) obj_t(std::move(obj));
    }

    explicit object(const obj_t &obj) {
      // Call constructor at the address of _obj
      _obj_p = new (_obj) obj_t(obj);
    }

    object(object &&other) : _obj_p(nullptr) {
      if(other.constructed()) {
        _obj_p = new (_obj) obj_t(std::move(other.get()));
      }
    }

    object(const object &other) : _obj_p(nullptr) {
      if(other.constructed()) {
        _obj_p = new (_obj) obj_t(other.get());
      }
    }
    
    object & operator = (object &&other) {
      if(other.constructed()) {
          *this = std::move(other.get());
      }
      else if(constructed()) {
        _obj_p->~obj_t();
        _obj_p = nullptr;
      }
      
      return *this;
    }

    // copy objects
    object & operator = (const object &other) {
      if(other.constructed()) {
        return (*this = other.get());
      }
      else if(constructed()) {
        _obj_p->~obj_t();
        _obj_p = nullptr;
      }

      return *this;
    }

    object & operator = (obj_t &&obj) {
        if(constructed()) {
            get() = std::move(obj); return *this;
        }
      
        _obj_p = new (_obj) obj_t(std::move(obj));
        
        return *this;
    }
      
    object & operator = (const obj_t &obj) {
      if(constructed()) {
        get() = obj; return *this;
      }
        
      _obj_p = new (_obj) obj_t(std::move(obj));
        
        return *this;
    }
      
    
    ~object() {
      if(constructed()) {
        get().~obj_t();
      }
    }
    
    const bool constructed() const { return _obj_p != nullptr; }
    
    const obj_t &get() const {
      return *_obj_p;
    }
    
    obj_t &get() {
      return *_obj_p;
    }
  };  
public:
  typedef T elem_t;
  
  object _obj;
  
  Optional() = default;
  
  Optional(elem_t &&val) : _obj(std::move(val)) {}
  Optional(const elem_t &val) : _obj(val) {}
  Optional(std::nullptr_t) : _obj() {}
  
  Optional &operator = (elem_t &&elem) { _obj = std::move(elem); return *this; }
  Optional &operator = (const elem_t &elem) { _obj = elem; return *this; }
  
  bool isEnabled() const {
    return _obj.constructed();
  }

  explicit operator bool () const {
    return isEnabled();
  }

  explicit operator bool () {
        return isEnabled();
  }
  
  operator elem_t&() {
    return _obj.get();
  }
  
  const elem_t* operator->() const {
    return &_obj.get();
  }
  
  const elem_t& operator*() const {
    return _obj.get();
  }

  elem_t* operator->() {
    return &_obj.get();
  }

  elem_t& operator*() {
    return _obj.get();
  }

  template<class X>
  bool operator==(const Optional<X> &other) const {
    if(other.isEnabled()) {
      return (*this == other._obj.get());
    }

    return !isEnabled();
  }

  template<class X>
  bool operator!=(const Optional<X> &other) const {
    return !(*this == other);
  }

  template<class X>
  bool operator>(const Optional<X> &other) const {
    return other.isEnabled() && (*this > other._obj.get());
  }

  template<class X>
  bool operator<(const Optional<X> &other) const {
    return !(*this >= other);
  }

  template<class X>
  bool operator>=(const Optional<X> &other) const {
    return (*this == other) || (*this > other);
  }

  template<class X>
  bool operator<=(const Optional<X> &other) const {
    return !(*this > other);
  }

  template<class X>
  bool operator==(const X &value) const {
    return isEnabled() && _obj.get() == value;
  }

  template<class X>
  bool operator!=(const X &value) const {
    return !(*this == value);
  }

  template<class X>
  bool operator<(const X &value) const {
    return !isEnabled() || _obj.get() < value;
  }

  template<class X>
  bool operator>(const X &value) const {
    return !(*this < value || *this == value);
  }

  template<class X>
  bool operator>=(const X &value) const {
    return !(*this < value);
  }

  template<class X>
  bool operator<=(const X &value) const {
    return !(*this > value);
  }
};
}
#endif // OPTIONAL_H
