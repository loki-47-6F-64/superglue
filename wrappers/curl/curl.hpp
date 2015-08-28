#pragma once

#include <tuple>
#include <vector>
#include <kitty/util/optional.h>
#include <kitty/util/utility.h>
#include <curl/curl.h>

namespace curl {
typedef util::safe_ptr<CURL, curl_easy_cleanup> sCURL;

typedef std::pair<std::string, std::string> Post;
typedef std::vector<Post> Posts;

class Init;

Init &init();

class http {
  sCURL _curl;
public:
  http();

  template<class ...SockOPtArgs>
  void sockopt(CURLoption option, SockOPtArgs && ... args) {
    curl_easy_setopt(_curl.get(), option, std::forward<SockOPtArgs>(args)...);
  }

  util::Optional<std::string> connect(const std::string &url, const Posts &posts);
private:
  long _retCode();

  CURLcode _perform();
};

}

