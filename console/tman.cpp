#include <iostream>

#include <kitty/util/auto_run.h>
#include <kitty/file/tcp.h>

#include <nlohmann/json.hpp>
#include <kitty/log/log.h>

#include "TMan.h"
#include "ice_trans.h"
#include "pool.h"
#include "pack.h"

#include <dlfcn.h>
#include <jni.h>

JavaVM *pj_jni_jvm { nullptr };

typedef int (*JNI_CreateJavaVM_t)(void *, void *, void *);
typedef jint (*registerNatives_t)(JNIEnv* env, jclass clazz);

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

int main(int argc, char* argv[]) {
  auto libandroid_runtime_dso = dlopen("libandroid_runtime.so", RTLD_NOW);
  auto libart = dlopen("libart.so", RTLD_NOW);

  if(!(libandroid_runtime_dso && libart)) {
    std::cerr << "cannot load [libandroid_runtime_dso] || [libart.so]" << std::endl;

    return 120;
  }

  
  auto JNI_CreateJavaVM = (JNI_CreateJavaVM_t) dlsym(libart, "JNI_CreateJavaVM");
  

  std::cout << "hello world!" << std::endl;
  JNIEnv *env { nullptr };

  JavaVMInitArgs vm_args;
  JavaVMOption jvmopt[4];
  jvmopt[0].optionString = "-Djava.class.path=/data/local/tmp/target-app.apk";
  jvmopt[1].optionString = "-agentlib:jdwp=transport=dt_android_adb,suspend=n,server=y";
  jvmopt[2].optionString = "-Djava.library.path=/data/local/tmp";
  jvmopt[3].optionString = "-verbose:jni"; // may want to remove this, it's noisy

  //JNI_GetDefaultJavaVMInitArgs(&vm_args);
  vm_args.nOptions = 4;
  vm_args.ignoreUnrecognized = JNI_FALSE;
  vm_args.version = JNI_VERSION_1_6;
  vm_args.options = jvmopt;

  
  std::cout << "hello world!" << std::endl;


  JNI_CreateJavaVM(&pj_jni_jvm, &env, &vm_args);

  std::cout << "hello world!" << std::endl;

  util::safe_ptr<JavaVM, jvm_destroy> jvm(pj_jni_jvm);


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
      }
    });
  });

  auto ice_trans = pool.ice_trans(on_data, on_ice_compl, on_call_connect);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  ice_trans.init_ice();



  auto json = pack({ ice_trans.credentials(), ice_trans.get_candidates() });
  auto str = json->dump();

  auto client = file::connect("localhost", "2345");

  if(!client.is_open()) {
    print(error, "Could not connect to localhost:2345 :: ", err::current());

    return 7;
  }

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

// linker flags needed :: -Wl,--export-dynamic
// Include all of the Android's libsigchain symbols
// libsigchain calls abort()
extern "C" {
JNIEXPORT void InitializeSignalChain() {}
JNIEXPORT void ClaimSignalChain() {}
JNIEXPORT void UnclaimSignalChain() {}
JNIEXPORT void InvokeUserSignalHandler() {}
JNIEXPORT void EnsureFrontOfChain() {}
JNIEXPORT void AddSpecialSignalHandlerFn() {}
JNIEXPORT void RemoveSpecialSignalHandlerFn() {}
}
