#ifndef DOSSIER_SSL_H
#define DOSSIER_SSL_H

#include <string>
#include <memory>

#include <openssl/ssl.h>

#include <kitty/ssl/ssl_stream.h>
#include <kitty/util/utility.h>

namespace ssl {

typedef util::safe_ptr<X509, X509_free> Certificate;
typedef util::safe_ptr<SSL_CTX, SSL_CTX_free> Context;

// On failure Context.get() returns nullptr
Context init_ctx_server(const char *caPath, const char *certPath, const char *keyPath, bool verify = false);
Context init_ctx_server(std::string& caPath, std::string& certPath, std::string& keyPath, bool verify = false);
Context init_ctx_server(std::string&& caPath, std::string&& certPath, std::string&& keyPath, bool verify = false);

Context init_ctx_client(const char *caPath, const char *certPath, const char *keyPath);
Context init_ctx_client(std::string& caPath, std::string& certPath, std::string& keyPath);
Context init_ctx_client(std::string&& caPath, std::string&& certPath, std::string&& keyPath);

Context init_ctx_client(const char *caPath);
Context init_ctx_client(std::string& caPath);
Context init_ctx_client(std::string&& caPath);

void init();
// On failure Client.get() returns nullptr
class sockaddr;
file::ssl accept(Context &ctx, int fd);
file::ssl connect(Context &ctx, const char *hostname, const char* port);

std::string getCN(const SSL *ssl);
}
#endif
