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
    object(obj_t &&obj) {
      // Call constructor at the address of _obj
      _obj_p = new (_obj) obj_t(std::move(obj));
    }

    object(const obj_t &obj) {
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
    
    // Swap objects
    object & operator = (object &&other) {
      if(other.constructed()) {
        if(constructed()) {
          get() = std::move(other.get());
        } else {
          _obj_p = new (_obj) obj_t(std::move(other.get()));
        }
      }
      else if(constructed()) {
        other._obj_p = new (other._obj) obj_t(std::move(get()));
      }
      
      return *this;
    }

    // copy objects
    object & operator = (const object &other) {
      if(other.constructed()) {
        if(constructed()) {
          get() = other.get();
        } else {
          _obj_p = new (_obj) obj_t(other.get());
        }
      }

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
  
  elem_t &operator = (elem_t &&elem) { _obj = std::move(elem); return _obj.get(); }
  elem_t &operator = (const elem_t &elem) { _obj = elem; return _obj.get(); }
  
  bool isEnabled() const {
    return _obj.constructed();
  }
  
  explicit operator bool () const {
    return _obj.constructed();
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
};
}
#endif // OPTIONAL_H
