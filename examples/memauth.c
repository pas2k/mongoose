#include <stdio.h>
#include <string.h>
#include "mongoose.h"

static int begin_request_handler(struct mg_connection *conn) {
  const struct mg_request_info *request_info = mg_get_request_info(conn);
  char content[100];
  int content_length = snprintf(content, sizeof(content),
                                "Hello from mongoose! Remote port: %d",
                                request_info->remote_port);
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
            content_length, content);
  return 1;
}

char broken_auth_file[8192] = {}; // Zero initialized

static const char * open_file_handler(const struct mg_connection *conn,
                             const char *path, size_t *data_len) {
  // Passwords for users: 123
  // Hashes generated with `echo -n test:mydomain.com:123 | md5sum`
  char *authFile = NULL;
  if (strcmp(path, "/security/file1") == 0) {
    authFile = "test1:mydomain.com:d3353d902be07ac36d0ee449c6e4d6ff\ntest2:mydomain.com:bfdfe32d5d2226f107452fe9869c569b";
  } else if (strcmp(path, "/security/file2") == 0) {
    authFile = "test2:mydomain.com:bfdfe32d5d2226f107452fe9869c569b\ntest3:mydomain.com:57046c3233af243b8ed070ac8da3e370\n";
  }
  if (authFile != NULL)
    *data_len = strlen(authFile);
  if (strcmp(path, "/security/file3") == 0) {
    authFile = broken_auth_file;
    *data_len = sizeof(broken_auth_file);
  }
  return authFile;
}



int main(void) {
  struct mg_context *ctx;
  struct mg_callbacks callbacks;
  char *userpassword3 = "\ntest1:mydomain.com:d3353d902be07ac36d0ee449c6e4d6ff\ntest4:mydomain.com:187012abfbf99564cb32c79f4473b20f"; 
  memcpy(broken_auth_file + sizeof(broken_auth_file) - strlen(userpassword3), userpassword3, strlen(userpassword3)); // No \0 or \n at the end

  // List of options. Last element must be NULL.
  const char *options[] = {"listening_ports", "8080",
                           "protect_uri", "/test1=/security/file1,/test2=/security/file2,/test3=/security/file3",
                           NULL};

  // Prepare callbacks structure. We have only one callback, the rest are NULL.
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.begin_request = begin_request_handler;
  callbacks.open_file = open_file_handler;

  // Start the web server.
  ctx = mg_start(&callbacks, NULL, options);

  // Wait until user hits "enter". Server is running in separate thread.
  // Navigating to http://localhost:8080 will invoke begin_request_handler().
  getchar();

  // Stop the server.
  mg_stop(ctx);

  return 0;
}
