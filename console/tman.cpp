#include <iostream>

#include <kitty/util/auto_run.h>
#include <kitty/file/tcp.h>

#include <nlohmann/json.hpp>
#include <kitty/log/log.h>

#include "TMan.h"
#include "ice_trans.h"
#include "pool.h"
#include "pack.h"

#include <jni.h>
#include <dlfcn.h>

#include "vm.h"
JavaVM *pj_jni_jvm { nullptr };

int distance(const int &l, const int &r) {
  return r - l;
}

void print_tman(const TMan<int> &tman) {
  std::cout << tman.get_descr().rank << ' ';

  auto peers = tman.get_peers();
  auto it = std::begin(peers);

  while(it != std::end(peers) && tman.get_descr().rank > it->rank) {
    ++it;
  }

  if(it == std::end(peers)) {
    return;
  }

  print_tman(*it->address);
};

void tmans_iterate(std::vector<TMan<int>> &tmans) {
  for(auto & tman : tmans) {
    tman.iterate();
    tman.rand_iterate();
  }
}

auto on_data = [](pj::ICECall call, std::string_view data) noexcept {
  std::cout << call.ip_addr.ip << ":" << call.ip_addr.port << " [><] " << data << std::endl;
};

auto on_ice_compl = [](pj::ice_trans_op_t op, pj::status_t status) noexcept {};

auto on_call_connect = [](pj::ICECall call) {
  print(debug, "<<<<<<<<<<<<<<<<<<<<< sending data >>>>>>>>>>>>>>>>>>>>>>>");

  call.send(util::toContainer("Hello peer!"));
};

void jvm_destroy(JavaVM *jvm) {
  jvm->DestroyJavaVM();
}

class except_t {
public:
  std::string_view name;
  std::string_view message;

  except_t() = default;

  except_t(const std::string_view &name, const std::string_view &message, const std::tuple<JNIEnv*, jstring, jstring> &env) :
    name(name), message(message), _env(env) {}

  except_t(except_t &&other) {
    _env = other._env;
    other._env = {nullptr, nullptr, nullptr};
  }

  except_t &operator=(except_t &&other) {
    std::swap(_env, other._env);
    std::swap(name, other.name);
    std::swap(message, other.message);

    return *this;
  }


  ~except_t() {
    if(std::get<0>(_env)) {
      std::get<0>(_env)->ReleaseStringUTFChars(std::get<1>(_env), name.data());
      std::get<0>(_env)->ReleaseStringUTFChars(std::get<2>(_env), message.data());
    }
  }

private:
  std::tuple<JNIEnv*, jstring, jstring> _env = {nullptr, nullptr, nullptr};
};

except_t get_message(JNIEnv *env, jthrowable except) {
  env->ExceptionClear();

  auto except_class = env->GetObjectClass(except);
  auto class_       = env->FindClass("java/lang/Class");

  auto get_name    = env->GetMethodID(class_, "getName", "()Ljava/lang/String;");
  auto get_message = env->GetMethodID(except_class, "getMessage", "()Ljava/lang/String;");

  auto jname    = (jstring)env->CallObjectMethod(except_class, get_name);
  auto jmessage = (jstring)env->CallObjectMethod(except, get_message);


  auto message = env->GetStringUTFChars(jmessage, 0);
  auto name    = env->GetStringUTFChars(jname, 0);

  return {
    {name},
    {message},
    {env,jname,jmessage}
  };
}

void handle_exception(JNIEnv *jni_env) {
  if(auto exception = jni_env->ExceptionOccurred()) {
    auto ex = get_message(jni_env, exception);
  }
  else {
    print(error, "no exception :|");
  }
}

int main(int argc, char* argv[]) {
  JNIEnv *jni_env { nullptr };
  util::safe_ptr<JavaVM, jvm_destroy> jvm {
    init_jvm(&pj_jni_jvm, &jni_env)
  };

  pj::init(nullptr);

  pj::Pool pool("Loki-ICE");

  pool.dns_resolv().set_ns({ "8.8.8.8" });
  pool.set_stun({ "stun.l.google.com", 19302 });

  util::AutoRun<void> auto_run;

  std::thread worker_thread([&pool, &auto_run]() {
    auto thread_ptr = pj::register_thread();

    auto_run.run([&pool]() {

      std::chrono::milliseconds max_tm { 500 };
      std::chrono::milliseconds milli { 0 };

      pool.timer_heap().poll(milli);

      milli = std::min(milli, max_tm);

      auto c = pool.io_queue().poll(milli);
      if(c < 0) {
        std::cerr << __FILE__ "::" << pj::err(pj::get_netos_err());

        std::abort();
      }
    });
  });

  auto ice_trans = pool.ice_trans(on_data, on_ice_compl, on_call_connect);

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  ice_trans.init_ice();


  auto json = pack({ ice_trans.credentials(), ice_trans.get_candidates() });
  auto str = json->dump();

  auto client = file::connect("192.168.0.115", "2345");

  if(!client.is_open()) {
    print(error, "Could not connect to 192.168.0.115:2345 :: ", err::current());

    return 7;
  }

  print(info, "Connected to 192.168.0.115:2345");
  util::append_struct(client.get_write_cache(), util::endian::little((std::uint16_t)str.size()));
  _print(client, str);


  auto size = util::endian::little(*util::read_struct<std::uint16_t>(client));

  if(size <= 2) {
    client.seal();

    worker_thread.join();

    return 0;
  }

  str.clear();
  str.reserve(size);
  client.eachByte([&str](auto byte) {
    str += byte;

    return err::OK;
    }, size);

  print(info, "-------------------------- ", str);
  auto remote = unpack(nlohmann::json::parse(str));

  ice_trans.set_role(pj::ice_sess_role_t::PJ_ICE_SESS_ROLE_CONTROLLING);
  auto err_code = ice_trans.start_ice(*remote);
  if(err_code) {
    print(error, err::current());

    auto_run.stop();
    worker_thread.join();

    return 7;
  }
  worker_thread.join();
  return 0;
}
/*
int main() {
  std::random_device r;
  std::default_random_engine engine { r() };

  std::uniform_int_distribution<int> rand(0, 1000);

  std::size_t C = 4;
  constexpr std::size_t size = 100;
  std::size_t iterations = 15;

  std::vector<TMan<int>> tmans;
  tmans.reserve(size);

  std::array<std::size_t, size> seq { };
  rand_seq(seq, engine);

  for(std::size_t x = 0; x < size; ++x) {
    tmans.emplace_back(seq[x], engine, distance, C);
  }

  std::sort(std::begin(tmans), std::end(tmans), [](auto& l, auto &r) { return l.get_descr().rank < r.get_descr().rank; });

  std::uniform_int_distribution<int> rand_el(0, (int)tmans.size() -1);

  auto rand_peers = [&tmans, &engine, &rand_el, C](int x) {
    std::vector<TMan<int>::descriptor_t> peers;
    for(std::size_t y = 0; y < C; ++y) {
      int el;

      do {
        el = rand_el(engine);
      } while(el == x);

      peers.emplace_back(tmans[el].get_descr());
    }

    return peers;
  };

  for(auto x = 0; x < size; ++x) {
    tmans[x].set_peers(rand_peers(x));
    tmans[x].set_rand_peers(rand_peers(x));
  }

  print_tman(tmans.front());
  std::cout << std::endl;

  for(auto x = 0; x < iterations; ++x) {
    tmans_iterate(tmans);

    print_tman(tmans.front());
    std::cout << std::endl;
  }

  std::cout << std::endl;
  for(auto &tman : tmans) {
    std::cout << tman.get_descr().rank << ' ';
  }
  std::cout << std::endl;
  return 0;
}
 */
