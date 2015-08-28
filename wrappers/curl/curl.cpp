#include "curl.hpp"


static std::size_t write_callback(char *ptr, std::size_t size, std::size_t nmemb, void *userdata) {
  std::string &output = *(std::string*)userdata;

  auto begin = ptr;
  auto end   = ptr + size * nmemb;

  output.append(begin, end);

  return size * nmemb;
}
// static size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream);

namespace curl {
typedef util::safe_ptr<curl_httppost, curl_formfree> sForm;

static sForm fromPosts(const Posts &posts) {
  curl_httppost *first { nullptr };
  curl_httppost *last  { nullptr };

  if(!posts.empty()) {
    for(auto &post : posts) {
      curl_formadd(
        &first,
        &last,
        CURLFORM_COPYNAME, post.first.c_str(),
        CURLFORM_COPYCONTENTS, post.second.c_str(),
        CURLFORM_END
      );
    }
  }

  return sForm { first };
}

class Init {
public:
  Init()  { curl_global_init(CURL_GLOBAL_ALL); }
  ~Init() { curl_global_cleanup(); }
};

Init& init() {
  static Init init_;

  return init_;
}

http::http() : _curl(curl_easy_init()) { }
util::Optional<std::string> http::connect(const std::string &url, const Posts &posts) {
  std::string buf;

  sockopt(CURLOPT_NOSIGNAL, 1L);
  sockopt(CURLOPT_WRITEDATA, &buf);
  sockopt(CURLOPT_WRITEFUNCTION, write_callback);
  sockopt(CURLOPT_FOLLOWLOCATION, 1L);
  sockopt(CURLOPT_URL, url.c_str());

  auto form = fromPosts(posts);
  if(form) {
    sockopt(CURLOPT_HTTPPOST, form.get());
  }

  CURLcode ret_code = _perform();

  if(ret_code) {
    err::set(curl_easy_strerror(ret_code));

    return {};
  }

  long code = _retCode();
  if(code < 200 || code >= 300) {
    err::set("status code [" + std::to_string(code) + ']');

    return {};
  }

  return std::move(buf);
}

long http::_retCode() {
  long code;

  curl_easy_getinfo(_curl.get(), CURLINFO_RESPONSE_CODE, &code);

  return code;
}

CURLcode http::_perform() {
  return curl_easy_perform(_curl.get());
}
}