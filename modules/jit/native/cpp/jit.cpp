#include <sys/mman.h>

#include <cassert>

#include <jit_interface.hpp>
#include <kitty/util/utility.h>
#include <kitty/util/set.h>
#include <kitty/util/string.h>
#include <config.hpp>

#include <log_severity.hpp>
#include "jit.hpp"

namespace jit {
using vec_code = std::vector<uint8_t>;

class map_p {
  void *_p;

  const std::size_t size;
public:
  template<class Func>
  Func get() { return reinterpret_cast<Func>(_p); }

  map_p(map_p &&p) : _p(p.get<void*>()), size(p.size) {
    p._p = nullptr;
  }

  map_p(std::size_t size) : _p(mmap(nullptr, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)), size(size) {}

  template<class It>
  map_p(const It begin, const It end) : map_p(std::distance(begin, end)) {
    std::copy(begin, end, get<uint8_t*>());
  }

  ~map_p() {
    munmap(_p, size);
  }
};

template<std::size_t N, bool s>
struct type_size {};

template<>
struct type_size<1, false> {
  using type = uint16_t;
};

template<>
struct type_size<2, false> {
  using type = uint16_t;
};

template<>
struct type_size<3, false> {
  using type = uint32_t;
};

template<>
struct type_size<4, false> {
  using type = uint32_t;
};

template<>
struct type_size<1, true> {
  using type = int16_t;
};

template<>
struct type_size<2, true> {
  using type = int16_t;
};

template<>
struct type_size<3, true> {
  using type = int32_t;
};

template<>
struct type_size<4, true> {
  using type = int32_t;
};

template<bool B, class T, class F>
struct conditional {
  typedef typename T::type type;
};

template<class T, class F>
struct conditional<false, T, F> {
  typedef typename F::type type;
};

template<bool B, class T, class F>
using conditional_t = typename conditional<B, T, F>::type;

template<class T> struct id { typedef T type; };

template<class T>
using decay_enum_t = conditional_t<std::is_enum<T>::value, std::underlying_type<T>, id<T>>;

template<class T>
auto decay_enum(T &&t) {
  return static_cast<decay_enum_t<std::decay_t<T>>>(t);
}

template<class T>
uint8_t to_byte(const T &t) { return static_cast<uint8_t>(t); }

template<class T>
uint16_t to_hword(const T &t) { return static_cast<uint16_t>(t); }

template<class T>
uint32_t to_word(const T &t) { return static_cast<uint32_t>(t); }

template<class T>
uint64_t to_dword(const T &t) { return static_cast<uint64_t>(t); }

template<class T>
T lshift(const T& val, const int l) { return (T)(((decay_enum_t<T>)val) << l); };

template<class T>
T rshift(const T& val, const int r) { return (T)(((decay_enum_t<T>)val) >> r); };

template<class T>
T _add(T &&t) {
  return t;
}

template<class T, class... Args>
std::decay_t<T> _add(T &&t, Args && ...args) {
  using primitive = std::decay_t<T>;

  return static_cast<primitive>(static_cast<decay_enum_t<primitive>>(t) + decay_enum(_add(std::forward<Args>(args)...)));
};

template<class T>
T _sub(T &&t) {
  return t;
}

template<class T, class... Args>
T _sub(T &&t, Args && ...args) {
  return static_cast<T>(static_cast<decay_enum_t<T>>(t) - decay_enum(_sub(std::forward<Args>(args)...)));
};

template<std::size_t bits, bool s>
class pseudo_t {
  using _type = typename type_size<bits / 8 + (bits % 8 ? 1 : 0), s>::type;
  static constexpr std::size_t bytes = sizeof(_type);

  _type _val;
public:
  pseudo_t() = default;
  pseudo_t(const pseudo_t &) = default;

  pseudo_t(const _type val) {
    assert(rshift(abs(val), bits) == 0);

    // val can't be lower than 0 when s = false
    if(val < 0) {
      // preserve sign
      _val = val | lshift(0x01, (bytes *8) - bits);
    }
    else {
      _val = val;
    }
  }

  operator _type & () { return _val; }
  explicit operator uint8_t () { return (uint8_t)_val; }

  operator const _type & () const { return _val; }
  explicit operator const uint8_t () const  { return (uint8_t)_val; }

  pseudo_t &operator = (const pseudo_t &) = default;
};

// From [load] to [pre_offset], a 0 means the opposite. [0] for load means write
enum struct mmem_mode : uint8_t {
  none = 0x00,
  load = 0x01,
  write_back = 0x02,
  transfer_byte = 0x04,
  PSR_or_force_user_mode = 0x04, // Load bundle : Can only be used in priveleged mode. Put it there for clarity
  inc_addr = 0x08,
  pre_offset = 0x10
};

mmem_mode operator | (const mmem_mode &l, const mmem_mode &r) {
  return static_cast<mmem_mode>(static_cast<uint8_t>(l) | static_cast<uint8_t>(r));
}

mmem_mode operator & (const mmem_mode &l, const mmem_mode &r) {
  return static_cast<mmem_mode>(static_cast<uint8_t>(l) & static_cast<uint8_t>(r));
}

enum struct reg : uint8_t {
  r0, r1, r2, r3, r4, r5, r6, r7, // The same across CPU modes
  r8, r9, r10, r11, r12, // 32-bit instructions, no 16-bit instructions
  SP, // Stack Pointer
  LR, // Link Register
  PC, // Program Counter
  dummy
};

enum struct cond : uint8_t {
  eq, // equal
  ne, // not equal
  cs, // unsigned higher or same
  cc, // unsigned lower
  mi, // negative
  pl, // positive or zero
  vs, // overflow
  vc, // no overflow
  hi, // unsigned higher
  ls, // unsigned lower or same
  ge, // greater or equal
  lt, // less than
  gt, // greater than
  le, // less than or equal
  al, // always
  no_choice // if opcode doesn't have the option.
};

enum struct data_op : uint8_t {
  AND,
  EOR,
  SUB,
  RSB,
  ADD,
  ADC,
  SBC,
  RSC,
  TST,
  TEQ,
  CMP,
  CMN,
  ORR,
  MOV,
  BIC,
  MVN
};

enum struct mul_op : uint8_t {
  mul = 0x00, // multiply two 32-bit values, store least signifant 32-bits of result
  mla = 0x02, // multiply two 32-bit values + one 32-bit value, store least signifant 32-bits of result
  umaal = 0x04, // multiplies two unsigned 32-bit values to produce a 64-bit value, adds two unsigned 32-bit values, and writes the 64-bit result to two registers.
  mls = 0x06, // like mla, but with subtraction, no updating flags
  umull = 0x08, // multiply two uint32_t values to 64-bit
  umlal = 0x0A, // multiple two uint32_t values to 64-bit, add 64-bit value
  smull = 0x0C, // multiply two int32_t values to 64-bit
  smlal = 0x0E, // multiple two int32_t values to 64-bit, add 64-bit value
};

struct shift_op {
  enum shift_type : uint8_t {
    logic_left,
    logic_right,
    arithmatic_right,
    rotate_right
  };
  uint8_t code;

  shift_op(const shift_op &) = default;

  // Shift by value of the leas-significant byte in [rin]
  shift_op(const reg rin, const shift_type type) : code(_add((uint8_t)lshift(rin, 4), lshift(type, 1), 1)) {}

  // Shift by immediate
  shift_op(const pseudo_t<5, false> c, const shift_type type) : code(_add(lshift((uint8_t)c, 3), lshift(type, 1))) {}

  // no_shift
  shift_op(std::nullptr_t) : code(0) {}
};

vec_code data(const cond c, const data_op op, const reg rout, const reg rin1, const reg rin2, const shift_op shift =
              shift_op(nullptr), const bool set_flags = false) {

  // These instructions only update flags in the CSPR
  assert(!(to_byte(op) >= to_byte(data_op::TST) && to_byte(op) <= to_byte(data_op::CMN)) || set_flags);

  return {
    (uint8_t)_add(rin2, lshift(shift.code, 4)),
    _add(rshift(shift.code, 4), lshift(rout, 4)),
    (uint8_t)_add(rin1, lshift(op, 5), lshift((set_flags ? 1 : 0), 4)),
    (uint8_t)_add(rshift(op, 3), lshift(c, 4))
  };
}

vec_code data(const cond c, const data_op op, const reg rout, const reg rin, const uint8_t immediate, const pseudo_t<4, false> rotate = 0, const bool set_flags = false) {
  // These instructions only update flags in the CSPR
  assert(!(to_byte(op) >= to_byte(data_op::TST) && to_byte(op) <= to_byte(data_op::CMN)) || set_flags);

  uint8_t rotate_ = rotate;
  return {
    immediate,
    _add(rotate_, lshift(rout, 4)),
    (uint8_t)_add(rin, lshift(op, 5), lshift((set_flags ? 1 : 0), 4)),
    (uint8_t)_add(rshift(op, 3), 0x02, lshift(c, 4))
  };
}

vec_code multiply(const cond c, const mul_op op, const reg rdH, const reg rdL, const reg rn, const reg rm, bool set_flags) {
  assert(util::all({ rdH, rdL, rm, rn }, [](const reg r) { return r != reg::PC; }));
  assert(rdH != rdL);


  return {
    (uint8_t)_add(rn, 0x90),
    (uint8_t)_add(rm, lshift(rdL, 4)),
    (uint8_t)_add(rdH, lshift(_add(op, set_flags ? 1 : 0), 4)),
    (uint8_t)lshift(c, 4)
  };
}

vec_code divide(const cond c, const reg rout, const reg rin_dividend, const reg rin_divisor, const bool s) {
  assert(util::all({ rout, rin_dividend, rin_divisor}, [](const reg r) { return r != reg::PC; }));

  return {
    (uint8_t)_add(rin_dividend, 0x10),
    (uint8_t)_add(rin_divisor, 0xF0),
    (uint8_t)_add(rout, lshift(s ? 1 : 3, 4)),
    (uint8_t)_add(lshift(c, 4), 0x07)
  };
}

// load from address at rin, address at [begin] must be lower than address at [end]
vec_code mmem(const cond c, const reg rin, const reg begin, const reg end, const mmem_mode mode) {
  assert(begin < end);

  // A store can't handle the [pc]
  assert(rin != reg::PC || to_byte(mode & mmem_mode::load));

  uint16_t store = 0;

  for(uint16_t x = to_hword(begin); x <= to_hword(end); ++x) {
    store |= lshift(0x01u, x);
  }

  return {
    (uint8_t)store,
    (uint8_t)rshift(store, 8),
    (uint8_t)_add(lshift(mode, 4), rin),
    (uint8_t)_add(lshift(c, 4), 0x08, rshift(mode, 4)),
  };
}

vec_code mmem_s(const cond c, const reg base, const reg rd, const reg rm, const mmem_mode mode, const shift_op shift = nullptr) {
  return {
    (uint8_t)_add(rm, lshift(shift.code, 4)),
    _add(rshift(shift.code, 4), lshift(rd, 4)),
    (uint8_t)_add(lshift(mode, 4), base),
    (uint8_t)_add(lshift(c, 4), 0x06, rshift(mode, 4))
  };
}

vec_code mmem_s(const cond c, const reg base, const reg rd, const pseudo_t<12, false> offset, const mmem_mode mode) {
  uint16_t offset_ = offset;
  return {
    (uint8_t)offset_,
    (uint8_t)_add(rshift(offset_, 4), lshift(rd, 4)),
    (uint8_t)_add(lshift(mode, 4), base),
    (uint8_t)_add(lshift(c, 4), 0x04, rshift(mode, 4))
  };
}

vec_code udiv(const reg rout, const reg rin_dividend, const reg rin_divisor, const cond c = cond::al) {
  return divide(c, rout, rin_dividend, rin_divisor, false);
}

vec_code sdiv(const reg rout, const reg rin_dividend, const reg rin_divisor, const cond c = cond::al) {
  return divide(c, rout, rin_dividend, rin_divisor, true);
}

vec_code mul(const reg rout, const reg rin1, const reg rin2, const cond c = cond::al) {
  return multiply(c, mul_op::mul, rout, reg::dummy, rin1, rin2, false);
}

vec_code mla(const reg rout, const reg rin1, const reg rin2, const reg ra, const cond c = cond::al) {
  return multiply(c, mul_op::mla, rout, ra, rin1, rin2, false);
}

vec_code mls(const reg rout, const reg rin1, const reg rin2, const reg rs, const cond c = cond::al) {
  return multiply(c, mul_op::mls, rout, rs, rin1, rin2, false);
}

vec_code umlaal(const reg rdH, const reg rdL, const reg rin1, const reg rin2, const cond c = cond::al) {
  return multiply(c, mul_op::umaal, rdH, rdL, rin1, rin2, false);
}

vec_code umull(const reg rdH, const reg rdL, const reg rin1, const reg rin2, const cond c = cond::al) {
  return multiply(c, mul_op::umull, rdH, rdL, rin1, rin2, false);
}

vec_code umlal(const reg rdH, const reg rdL, const reg rin1, const reg rin2, const cond c = cond::al) {
  return multiply(c, mul_op::umlal, rdH, rdL, rin1, rin2, false);
}

vec_code smull(const reg rdH, const reg rdL, const reg rin1, const reg rin2, const cond c = cond::al) {
  return multiply(c, mul_op::smull, rdH, rdL, rin1, rin2, false);
}

vec_code smlal(const reg rdH, const reg rdL, const reg rin1, const reg rin2, const cond c = cond::al) {
  return multiply(c, mul_op::smlal, rdH, rdL, rin1, rin2, false);
}

vec_code ldm(const reg rin, const reg begin, const reg end, const mmem_mode mode, const cond c = cond::al) {
  assert(!(to_byte(mode & mmem_mode::load)));

  return mmem(c, rin, begin, end, mode);
}

vec_code stm(const reg rin, const reg begin, const reg end, const mmem_mode mod, const cond c = cond::al) {
  return mmem(c, rin, begin, end, mod | mmem_mode::load);
}

vec_code ldr(const reg base, const reg rd, const reg rm, const mmem_mode mode, const cond c = cond::al) {
  return mmem_s(c, base, rd, rm, mode | mmem_mode::load);
}

vec_code ldr(const reg rout, const reg base, const pseudo_t<12, false> offset, const mmem_mode mode = mmem_mode::none , const cond c = cond::al) {
  return mmem_s(c, base, rout, offset, mode | mmem_mode::load);
}

vec_code str(const reg base, const reg rd, const reg rm, const mmem_mode mode, const cond c = cond::al) {
  assert(!(to_byte(mode & mmem_mode::load)));

  return mmem_s(c, base, rd, rm, mode);
}

vec_code str(const reg rout, const reg base, const pseudo_t<12, false> offset, const mmem_mode mode = mmem_mode::none , const cond c = cond::al) {
  assert(!(to_byte(mode & mmem_mode::load)));

  return mmem_s(c, base, rout, offset, mode);
}

vec_code push(const reg begin, const reg end, const cond c = cond::al) {
  return ldm(reg::SP, begin, end, mmem_mode::write_back | mmem_mode::inc_addr, c);
}

vec_code pop(const reg begin, const reg end, const cond c = cond::al) {
  return stm(reg::SP, begin, end, mmem_mode::write_back | mmem_mode::pre_offset, c);
}

vec_code push(const reg rin, const cond c = cond::al) {
  return str(rin, reg::SP, 0, mmem_mode::write_back | mmem_mode::inc_addr, c);
}

vec_code pop(const reg rout, const cond c = cond::al) {
  return ldr(rout, reg::SP, 0, mmem_mode::write_back | mmem_mode::pre_offset, c);
}

vec_code mov(const reg rout, const uint8_t immediate, const cond c = cond::al) {
  assert(rout < reg::SP);

  // On move with immediate : operand ignored
  return data(c, data_op::MOV, rout, reg::dummy, immediate);
}

vec_code mov(const reg rout, const reg rin, const cond c = cond::al) {
  assert(rout < reg::SP);

  // On move with two operands : first operand ignored
  return data(c, data_op::MOV, rout, reg::dummy, rin);
}

vec_code add(const reg rout, const reg rin, const uint8_t immediate, const cond c = cond::al) {
  return data(c, data_op::ADD, rout, rin, immediate);
}

vec_code add(const reg rout, const reg rin1, const reg rin2, const cond c = cond::al) {
  return data(c, data_op::ADD, rout, rin1, rin2);
}

vec_code sub(const reg rin1, const reg rin2, const cond c = cond::al) {
  return data(c, data_op::CMP, reg::dummy, rin1, rin2, nullptr, true);
}

vec_code sub(const reg rin, const uint8_t immediate, const cond c = cond::al) {
  return data(c, data_op::CMP, reg::dummy, rin, immediate, 0, true);
}

vec_code cmp(const reg rin1, const reg rin2, const cond c = cond::al) {
  return data(c, data_op::CMP, reg::dummy, rin1, rin2, nullptr, true);
}

vec_code cmp(const reg rin, const uint8_t immediate, const cond c = cond::al) {
  return data(c, data_op::CMP, reg::dummy, rin, immediate, 0, true);
}

// store reg::PC + c at rout. Could be reg::PC
vec_code addr(const reg rout, const uint8_t immediate, const cond c = cond::al) {
  return add(rout, reg::PC, immediate, c);
}

// goto address at [rin]
vec_code bx(const reg rin, const cond c = cond::al) {
  return {
    (uint8_t)_add(0x10, rin), 0xFF, 0x2F, (uint8_t)_add(lshift(c, 4), 0x01) // bx [rin]
  };
};

vec_code bx(const pseudo_t<24, true> offset, const bool link_bit, const cond c = cond::al) {
  // Due to prefetching, offset goes two words to far
  const pseudo_t<24, true> offset_fixed(offset - 2);

  return {
    (uint8_t)offset_fixed, (uint8_t)rshift(offset_fixed, 8), (uint8_t)rshift(offset_fixed, 16), // offset
    (uint8_t)_add(lshift(c, 4), (link_bit ? 0x0B : 0x0A))
  };
}

vec_code ret() {
  return bx(reg::LR);
}

template<class T>
vec_code literal(const T& t) {
  constexpr std::size_t size = sizeof(T);
  constexpr std::size_t padding = size % sizeof(uint32_t);


  const uint8_t *p = reinterpret_cast<const uint8_t*>(&t);
  vec_code code;
  code.resize(size + padding);

  std::copy(p, p + size, std::begin(code));

  return code;
}

}

uint32_t constant() { return 42; }

uint32_t add2(uint32_t first, uint32_t second) { return first + second; }
uint64_t add1(uint32_t first) { return first + constant(); }

uint32_t arr_param30(uint32_t p[30]) { return util::fold(p, p + 30, [](uint32_t l, uint32_t r) { return l + r; }); }
uint32_t arr_param4(uint32_t p[4]) { return util::fold(p, p + 4, [](uint32_t l, uint32_t r) { return l + r; }); }

typedef uint32_t(*uint32_t_call_type)();
typedef uint32_t(*uint32_t_call_add_two_params)(uint32_t, uint32_t);
typedef uint64_t(*uint64_t_call_add_two_params)(uint32_t, uint32_t);
typedef uint32_t(*uint32_t_call_add_three_params)(uint32_t, uint32_t, uint32_t);
typedef uint64_t(*uint64_t_call_add_three_params)(uint64_t, uint32_t, uint32_t);

void gen::JitInterface::run() {
  auto return_const = util::copy_to<jit::map_p>(util::concat(
    {
      mov(jit::reg::r1, 14),
      add(jit::reg::r3, jit::reg::r1, 14),
      add(jit::reg::r2, jit::reg::r1, jit::reg::r3),
      mov(jit::reg::r0, jit::reg::r2),
      jit::ret()
    }
  ));

  auto add_two_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::push(jit::reg::r0, jit::reg::r1),
      jit::pop(jit::reg::r2, jit::reg::r3),
      jit::add(jit::reg::r0, jit::reg::r2, jit::reg::r3),
      jit::ret()
    }
  ));


  auto max_two_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::cmp(jit::reg::r0, jit::reg::r1),
      jit::mov(jit::reg::r0, jit::reg::r1, jit::cond::lt),
      jit::ret()
    }
  ));


  auto call_constant = util::copy_to<jit::map_p>(util::concat(
    {
      jit::push(jit::reg::LR),
      jit::ldr(jit::reg::r0, jit::reg::PC, 0x08, jit::mmem_mode::pre_offset | jit::mmem_mode::inc_addr),
      jit::bx(jit::reg::r0),
      jit::pop(jit::reg::LR),
      jit::ret(),
      jit::literal(reinterpret_cast<void*>(constant))
    }
  ));

  auto call_max_two_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::push(jit::reg::LR),
      jit::mov(jit::reg::r0, 33),
      jit::mov(jit::reg::r1, 42),
      jit::ldr(jit::reg::r2, jit::reg::PC, 0x08, jit::mmem_mode::pre_offset | jit::mmem_mode::inc_addr),
      jit::bx(jit::reg::r2),
      jit::pop(jit::reg::LR),
      jit::ret(),
      jit::literal(max_two_params.get<void*>())
    }
  ));

  auto branch = util::copy_to<jit::map_p>(util::concat(
    {
      jit::mov(jit::reg::r0, 42),
      jit::bx(0x03, false),
      jit::ret(),
      jit::mov(jit::reg::r0, 21),
      jit::bx(-2, false),
      jit::mov(jit::reg::r0, 21),
      jit::ret()
    }
  ));

  auto mul_two_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::mul(jit::reg::r0, jit::reg::r0, jit::reg::r1),
      jit::ret()
    }
  ));

  auto mul_three_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::mla(jit::reg::r0, jit::reg::r0, jit::reg::r1, jit::reg::r2),
      jit::ret()
    }
  ));

  auto umull_two_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::umull(jit::reg::r1, jit::reg::r0, jit::reg::r1, jit::reg::r0),
      jit::ret()
    }
  ));

  auto umlal_three_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::umlal(jit::reg::r1, jit::reg::r0, jit::reg::r2, jit::reg::r3),
      jit::ret()
    }
  ));

  auto div_two_params = util::copy_to<jit::map_p>(util::concat(
    {
      jit::udiv(jit::reg::r0, jit::reg::r0, jit::reg::r1),
      jit::ret()
    }
  ));

  uint32_t_call_type func = return_const.get<uint32_t_call_type>();
  uint32_t const_ret = func();
  logManager->log(LogSeverity::DEBUG, "const_ret = [" + std::to_string(const_ret) + "]");

  uint32_t add_two_params_res = (add_two_params.get<uint32_t_call_add_two_params>())(19, 23);
  logManager->log(LogSeverity::DEBUG, "add_two_params = [" + std::to_string(add_two_params_res) + "]");

  uint32_t max_two_params_res1 = (max_two_params.get<uint32_t_call_add_two_params>())(42, 13);

  uint32_t max_two_params_res2 = (max_two_params.get<uint32_t_call_add_two_params>())(13, 42);
  logManager->log(LogSeverity::DEBUG, "max_two_params = [" + std::to_string(max_two_params_res1) + "]");
  logManager->log(LogSeverity::DEBUG, "max_two_params = [" + std::to_string(max_two_params_res2) + "]");

  uint32_t call_constant_res = (call_constant.get<uint32_t_call_type>())();
  logManager->log(LogSeverity::DEBUG, "call_constant = [" + std::to_string(call_constant_res) + "]");

  uint32_t call_max_two_params_res = (call_max_two_params.get<uint32_t_call_type>())();
  logManager->log(LogSeverity::DEBUG, "call_max_two_params = [" + std::to_string(call_max_two_params_res) + "]");

  uint32_t branch_res = branch.get<uint32_t_call_type>()();
  logManager->log(LogSeverity::DEBUG, "branch = [" + std::to_string(branch_res) + "]");

  uint32_t mul_two_params_res = (mul_two_params.get<uint32_t_call_add_two_params>())(7, 6);
  logManager->log(LogSeverity::DEBUG, "mul_two_params = [" + std::to_string(mul_two_params_res) + "]");

  uint32_t mul_three_params_res = (mul_three_params.get<uint32_t_call_add_three_params>())(4, 5, 22);
  logManager->log(LogSeverity::DEBUG, "mul_three_params = [" + std::to_string(mul_three_params_res) + "]");

  uint64_t umull_two_params_res = (umull_two_params.get<uint64_t_call_add_two_params>())(7, 6);
  logManager->log(LogSeverity::DEBUG, "umull_two_params = [" + std::to_string(umull_two_params_res) + "]");

  uint64_t umlal_three_params_res = (umlal_three_params.get<uint64_t_call_add_three_params>())(22, 4, 5);
  logManager->log(LogSeverity::DEBUG, "mul_three_params = [" + std::to_string(umlal_three_params_res) + "]");

  uint32_t div_two_params_res = (div_two_params.get<uint32_t_call_add_two_params>())(84, 2);
  logManager->log(LogSeverity::DEBUG, "div_two_params = [" + std::to_string(div_two_params_res) + "]");
}
