// SPDX-FileCopyrightText: 2024 Cesanta Software Limited
// SPDX-License-Identifier: GPL-2.0-only or commercial
// Generated by Mongoose Wizard, https://mongoose.ws/wizard/

#include "mongoose.h"
#include "mongoose_glue.h"

#if !defined(HTTP_URL) && !defined(HTTPS_URL)
#if MG_ARCH == MG_ARCH_UNIX || MG_ARCH == MG_ARCH_WIN32
#define HTTP_URL "http://0.0.0.0:8080"
#define HTTPS_URL "https://0.0.0.0:8443"
#else
#define HTTP_URL "http://0.0.0.0:80"
#define HTTPS_URL "https://0.0.0.0:443"
#endif
#endif

#ifndef offsetof
#define offsetof(st, m) ((size_t) ((char *) &((st *) 0)->m - (char *) 0))
#endif

#define NO_CACHE_HEADERS "Cache-Control: no-cache\r\n"
#define JSON_HEADERS "Content-Type: application/json\r\n" NO_CACHE_HEADERS

// How to create a self signed Elliptic Curve certificate, see
// https://github.com/cesanta/mongoose/blob/master/test/certs/generate.sh
#define TLS_CERT                                                       \
  "-----BEGIN CERTIFICATE-----\n"                                      \
  "MIIBMTCB2aADAgECAgkAluqkgeuV/zUwCgYIKoZIzj0EAwIwEzERMA8GA1UEAwwI\n" \
  "TW9uZ29vc2UwHhcNMjQwNTA3MTQzNzM2WhcNMzQwNTA1MTQzNzM2WjARMQ8wDQYD\n" \
  "VQQDDAZzZXJ2ZXIwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASo3oEiG+BuTt5y\n" \
  "ZRyfwNr0C+SP+4M0RG2pYkb2v+ivbpfi72NHkmXiF/kbHXtgmSrn/PeTqiA8M+mg\n" \
  "BhYjDX+zoxgwFjAUBgNVHREEDTALgglsb2NhbGhvc3QwCgYIKoZIzj0EAwIDRwAw\n" \
  "RAIgTXW9MITQSwzqbNTxUUdt9DcB+8pPUTbWZpiXcA26GMYCIBiYw+DSFMLHmkHF\n" \
  "+5U3NXW3gVCLN9ntD5DAx8LTG8sB\n"                                     \
  "-----END CERTIFICATE-----\n"

#define TLS_KEY                                                        \
  "-----BEGIN EC PRIVATE KEY-----\n"                                   \
  "MHcCAQEEIAVdo8UAScxG7jiuNY2UZESNX/KPH8qJ0u0gOMMsAzYWoAoGCCqGSM49\n" \
  "AwEHoUQDQgAEqN6BIhvgbk7ecmUcn8Da9Avkj/uDNERtqWJG9r/or26X4u9jR5Jl\n" \
  "4hf5Gx17YJkq5/z3k6ogPDPpoAYWIw1/sw==\n"                             \
  "-----END EC PRIVATE KEY-----\n"

typedef void (*mongoose_data_func_t)(void *);
typedef bool (*mongoose_action_checker_t)(void);
typedef void (*mongoose_action_starter_t)(struct mg_str);
typedef void *(*mongoose_ota_opener_t)(char *, size_t);
typedef bool (*mongoose_ota_closer_t)(void *);
typedef bool (*mongoose_ota_writer_t)(void *, void *, size_t);
typedef void (*mongoose_custom_reply_t)(struct mg_connection *,
                                        struct mg_http_message *);
typedef void *(*mongoose_file_opener_t)(char *, size_t);
typedef void (*mongoose_file_server_t)(struct mg_connection *,
                                       struct mg_http_message *hm, char *);

struct mg_mgr g_mgr;  // Mongoose event manager

#if WIZARD_ENABLE_HTTP || WIZARD_ENABLE_HTTPS
// Every time device state changes, this counter increments.
// Used by the heartbeat endpoint, to signal the UI when to refresh
static unsigned long s_device_change_version = 0;

struct attribute {
  const char *name;
  const char *type;
  const char *format;
  size_t offset;
  size_t size;
  bool readonly;
};

struct apihandler {
  const char *name;
  const char *type;
  bool readonly;
  int read_level;
  int write_level;
  unsigned long version;
};

struct apihandler_custom {
  struct apihandler common;
  void (*reply)(struct mg_connection *, struct mg_http_message *);
};

struct apihandler_upload {
  struct apihandler common;
  void *(*opener)(char *, size_t);         // Open function (OTA and upload)
  bool (*closer)(void *);                  // Closer function (OTA and upload)
  bool (*writer)(void *, void *, size_t);  // Writer function (OTA and upload)
};

struct apihandler_file {
  struct apihandler common;
  mongoose_file_opener_t opener;
  mongoose_file_server_t server;
};

struct apihandler_ota {
  struct apihandler common;
  void *(*opener)(char *, size_t);         // Open function (OTA and upload)
  bool (*closer)(void *);                  // Closer function (OTA and upload)
  bool (*writer)(void *, void *, size_t);  // Writer function (OTA and upload)
};

struct apihandler_action {
  struct apihandler common;
  bool (*checker)(void);           // Checker function for actions
  void (*starter)(struct mg_str);  // Starter function for actions
};

struct apihandler_data {
  struct apihandler common;
  const struct attribute *attributes;  // Points to the strucure descriptor
  size_t data_size;                    // Size of C structure
  void (*getter)(void *);              // Getter/check/begin function
  void (*setter)(void *);              // Setter/start/end function
};

struct apihandler_array {
  struct apihandler common;
  const struct attribute *attributes;  // Points to the strucure descriptor
  size_t data_size;                    // Size of C structure
  void (*getter)(uint64_t, void *);    // Getter/check/begin function
  void (*setter)(uint64_t, void *);    // Setter/start/end function
  uint64_t (*sizer)(void);             // Array size, for data handlers only
};

struct attribute s_state_attributes[] = {
  {"speed", "int", NULL, offsetof(struct state, speed), 0, false},
  {"temperature", "int", NULL, offsetof(struct state, temperature), 0, false},
  {"humidity", "int", NULL, offsetof(struct state, humidity), 0, false},
  {"uptime", "int", NULL, offsetof(struct state, uptime), 0, false},
  {"version", "string", NULL, offsetof(struct state, version), 20, false},
  {"online", "bool", NULL, offsetof(struct state, online), 0, false},
  {"lights", "bool", NULL, offsetof(struct state, lights), 0, false},
  {"level", "int", NULL, offsetof(struct state, level), 0, false},
  {NULL, NULL, NULL, 0, 0, false}
};
struct attribute s_leds_attributes[] = {
  {"led1", "bool", NULL, offsetof(struct leds, led1), 0, false},
  {"led2", "bool", NULL, offsetof(struct leds, led2), 0, false},
  {"led3", "bool", NULL, offsetof(struct leds, led3), 0, false},
  {NULL, NULL, NULL, 0, 0, false}
};
struct attribute s_network_settings_attributes[] = {
  {"ip_address", "string", NULL, offsetof(struct network_settings, ip_address), 20, false},
  {"gw_address", "string", NULL, offsetof(struct network_settings, gw_address), 20, false},
  {"netmask", "string", NULL, offsetof(struct network_settings, netmask), 20, false},
  {"dhcp", "bool", NULL, offsetof(struct network_settings, dhcp), 0, false},
  {NULL, NULL, NULL, 0, 0, false}
};
struct attribute s_settings_attributes[] = {
  {"string_val", "string", NULL, offsetof(struct settings, string_val), 40, false},
  {"log_level", "string", NULL, offsetof(struct settings, log_level), 10, false},
  {"double_val", "double", "%.5f", offsetof(struct settings, double_val), 0, false},
  {"int_val", "int", NULL, offsetof(struct settings, int_val), 0, false},
  {"bool_val", "bool", NULL, offsetof(struct settings, bool_val), 0, false},
  {NULL, NULL, NULL, 0, 0, false}
};
struct attribute s_security_attributes[] = {
  {"admin_password", "string", NULL, offsetof(struct security, admin_password), 40, false},
  {"user_password", "string", NULL, offsetof(struct security, user_password), 40, false},
  {NULL, NULL, NULL, 0, 0, false}
};

struct apihandler_action s_apihandler_reboot = {{"reboot", "action", false, 3, 7, 0UL}, glue_check_reboot, glue_start_reboot};
struct apihandler_action s_apihandler_reformat = {{"reformat", "action", false, 3, 7, 0UL}, glue_check_reformat, glue_start_reformat};
struct apihandler_ota s_apihandler_firmware_update = {{"firmware_update", "ota", false, 3, 7, 0UL}, glue_ota_begin_firmware_update, glue_ota_end_firmware_update, glue_ota_write_firmware_update};
struct apihandler_upload s_apihandler_file_upload = {{"file_upload", "upload", false, 3, 7, 0UL}, glue_upload_open_file_upload, glue_upload_close_file_upload, glue_upload_write_file_upload};
struct apihandler_custom s_apihandler_graph_data = {{"graph_data", "custom", false, 3, 3, 0UL}, glue_reply_graph_data};
struct apihandler_data s_apihandler_state = {{"state", "data", true, 0, 0, 0UL}, s_state_attributes, sizeof(struct state), (void (*)(void *)) glue_get_state, NULL};
struct apihandler_data s_apihandler_leds = {{"leds", "data", false, 3, 3, 0UL}, s_leds_attributes, sizeof(struct leds), (void (*)(void *)) glue_get_leds, (void (*)(void *)) glue_set_leds};
struct apihandler_data s_apihandler_network_settings = {{"network_settings", "data", false, 3, 7, 0UL}, s_network_settings_attributes, sizeof(struct network_settings), (void (*)(void *)) glue_get_network_settings, (void (*)(void *)) glue_set_network_settings};
struct apihandler_data s_apihandler_settings = {{"settings", "data", false, 3, 7, 0UL}, s_settings_attributes, sizeof(struct settings), (void (*)(void *)) glue_get_settings, (void (*)(void *)) glue_set_settings};
struct apihandler_data s_apihandler_security = {{"security", "data", false, 7, 7, 0UL}, s_security_attributes, sizeof(struct security), (void (*)(void *)) glue_get_security, (void (*)(void *)) glue_set_security};
struct apihandler_custom s_apihandler_loglevels = {{"loglevels", "custom", false, 0, 0, 0UL}, glue_reply_loglevels};
struct apihandler_custom s_apihandler_events = {{"events", "custom", false, 0, 0, 0UL}, glue_reply_events};

static struct apihandler *s_apihandlers[] = {
  (struct apihandler *) &s_apihandler_reboot,
  (struct apihandler *) &s_apihandler_reformat,
  (struct apihandler *) &s_apihandler_firmware_update,
  (struct apihandler *) &s_apihandler_file_upload,
  (struct apihandler *) &s_apihandler_graph_data,
  (struct apihandler *) &s_apihandler_state,
  (struct apihandler *) &s_apihandler_leds,
  (struct apihandler *) &s_apihandler_network_settings,
  (struct apihandler *) &s_apihandler_settings,
  (struct apihandler *) &s_apihandler_security,
  (struct apihandler *) &s_apihandler_loglevels,
  (struct apihandler *) &s_apihandler_events
};

static struct apihandler *get_api_handler(struct mg_str name) {
  size_t num_handlers = sizeof(s_apihandlers) / sizeof(s_apihandlers[0]);
  size_t i;
  if (name.len == 0) return NULL;
  if (num_handlers == 0) return NULL;
  for (i = 0; i < num_handlers; i++) {
    struct apihandler *h = s_apihandlers[i];
    size_t n = strlen(h->name);
    if (n > name.len) continue;
    if (strncmp(name.buf, h->name, n) != 0) continue;
    if (name.len > n && name.buf[n] != '/') continue;
    return h;
  }
  return NULL;
}

static struct apihandler *find_handler(struct mg_http_message *hm) {
  if (hm->uri.len < 6 || strncmp(hm->uri.buf, "/api/", 5) != 0) return NULL;
  return get_api_handler(mg_str_n(hm->uri.buf + 5, hm->uri.len - 5));
}

void mg_json_get_str2(struct mg_str json, const char *path, char *buf,
                      size_t len) {
  struct mg_str s = mg_json_get_tok(json, path);
  if (s.len > 1 && s.buf[0] == '"') {
    mg_json_unescape(mg_str_n(s.buf + 1, s.len - 2), buf, len);
  }
}

void mongoose_set_http_handlers(const char *name, ...) {
  struct apihandler *h = get_api_handler(mg_str(name));
  va_list ap;
  va_start(ap, name);
  if (h == NULL) {
    MG_ERROR(("No API with name [%s]", name));
  } else if (strcmp(h->type, "data") == 0) {
    ((struct apihandler_data *) h)->getter = va_arg(ap, mongoose_data_func_t);
    ((struct apihandler_data *) h)->setter = va_arg(ap, mongoose_data_func_t);
  } else if (strcmp(h->type, "action") == 0) {
    ((struct apihandler_action *) h)->checker =
        va_arg(ap, mongoose_action_checker_t);
    ((struct apihandler_action *) h)->starter =
        va_arg(ap, mongoose_action_starter_t);
  } else if (strcmp(h->type, "file") == 0) {
    ((struct apihandler_file *) h)->opener = va_arg(ap, mongoose_file_opener_t);
    ((struct apihandler_file *) h)->server = va_arg(ap, mongoose_file_server_t);
  } else if (strcmp(h->type, "ota") == 0 || strcmp(h->type, "upload") == 0) {
    ((struct apihandler_ota *) h)->opener = va_arg(ap, mongoose_ota_opener_t);
    ((struct apihandler_ota *) h)->closer = va_arg(ap, mongoose_ota_closer_t);
    ((struct apihandler_ota *) h)->writer = va_arg(ap, mongoose_ota_writer_t);
  } else if (strcmp(h->type, "custom") == 0) {
    ((struct apihandler_custom *) h)->reply =
        va_arg(ap, mongoose_custom_reply_t);
  } else {
    MG_ERROR(("Setting [%s] failed: not implemented", name));
  }
  va_end(ap);
}

#if WIZARD_ENABLE_HTTP_UI_LOGIN

struct user {
  struct user *next;
  char name[32];   // User name
  char token[21];  // Login token
  int level;       // Access level
};

static int (*s_auth)(const char *, const char *) = glue_authenticate;
static struct user *s_users;  // List of authenticated users

void mongoose_set_auth_handler(int (*fn)(const char *, const char *)) {
  s_auth = fn;
}

// Parse HTTP requests, return authenticated user or NULL
static struct user *authenticate(struct mg_http_message *hm) {
  char user[100], pass[100];
  struct user *u, *result = NULL;
  mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));

  if (user[0] != '\0' && pass[0] != '\0') {
    // Both user and password is set, auth by user/password via glue API
    int level = s_auth(user, pass);
    MG_DEBUG(("user %s, level: %d", user, level));
    if (level > 0) {  // Proceed only if the firmware authenticated us
      // uint64_t uid = hash(3, mg_str(user), mg_str(":"), mg_str(pass));
      for (u = s_users; u != NULL && result == NULL; u = u->next) {
        if (strcmp(user, u->name) == 0) result = u;
      }
      // Not yet authenticated, add to the list
      if (result == NULL) {
        result = (struct user *) calloc(1, sizeof(*result));
        mg_snprintf(result->name, sizeof(result->name), "%s", user);
        mg_random_str(result->token, sizeof(result->token) - 1);
        result->level = level, result->next = s_users, s_users = result;
      }
    }
  } else if (user[0] == '\0' && pass[0] != '\0') {
    for (u = s_users; u != NULL && result == NULL; u = u->next) {
      if (strcmp(u->token, pass) == 0) result = u;
    }
  }
  MG_VERBOSE(("[%s/%s] -> %s", user, pass, result ? "OK" : "FAIL"));
  return result;
}

static void handle_login(struct mg_connection *c, struct user *u) {
  char cookie[256];
  mg_snprintf(cookie, sizeof(cookie),
              "Set-Cookie: access_token=%s; Path=/; "
              "%sHttpOnly; SameSite=Lax; Max-Age=%d\r\n",
              u->token, c->is_tls ? "Secure; " : "", 3600 * 24);
  mg_http_reply(c, 200, cookie, "{%m:%m,%m:%d}\n",  //
                MG_ESC("user"), MG_ESC(u->name),    //
                MG_ESC("level"), u->level);
}

static void handle_logout(struct mg_connection *c) {
  char cookie[256];
  mg_snprintf(cookie, sizeof(cookie),
              "Set-Cookie: access_token=; Path=/; "
              "Expires=Thu, 01 Jan 1970 00:00:00 UTC; "
              "%sHttpOnly; Max-Age=0; \r\n",
              c->is_tls ? "Secure; " : "");
  mg_http_reply(c, 401, cookie, "Unauthorized\n");
}
#endif  // WIZARD_ENABLE_HTTP_UI_LOGIN

struct upload_state {
  char marker;               // Tells that we're a file upload connection
  size_t expected;           // POST data length, bytes
  size_t received;           // Already received bytes
  void *fp;                  // Opened file
  bool (*fn_close)(void *);  // Close function
  bool (*fn_write)(void *, void *, size_t);  // Write function
};

struct action_state {
  char marker;       // Tells that we're an action connection
  bool (*fn)(void);  // Action status function
};

static void close_uploaded_file(struct upload_state *us) {
  us->marker = 0;
  if (us->fn_close != NULL && us->fp != NULL) {
    us->fn_close(us->fp);
    us->fp = NULL;
  }
  memset(us, 0, sizeof(*us));
}

static bool file_closer(void *p) {
  mg_fs_close((struct mg_fd *) p);
  MG_INFO(("AAAAAAAAA %p", p));
  return true;
}

static bool file_writer(void *p, void *buf, size_t len) {
  struct mg_fd *fd = (struct mg_fd *) p;
  size_t written = fd->fs->wr(fd->fd, buf, len);
  MG_INFO(("AAAAAAAAA %lu", written));
  return written == len;
}

static void upload_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct upload_state *us = (struct upload_state *) c->data;
  if (sizeof(*us) > sizeof(c->data)) {
    mg_error(
        c, "FAILURE: sizeof(c->data) == %lu, need %lu. Set -DMG_DATA_SIZE=XXX",
        sizeof(c->data), sizeof(*us));
    return;
  }
  // Catch uploaded file data for both MG_EV_READ and MG_EV_HTTP_HDRS
  if (us->marker == 'U' && ev == MG_EV_READ && us->expected > 0 &&
      c->recv.len > 0) {
    size_t alignment = 512;  // Maximum flash write granularity (iMXRT, Pico)
    size_t aligned = (us->received + c->recv.len < us->expected)
                         ? aligned = MG_ROUND_DOWN(c->recv.len, alignment)
                         : c->recv.len;  // Last write can be unaligned
    bool ok = aligned > 0 ? us->fn_write(us->fp, c->recv.buf, aligned) : true;
    us->received += aligned;
    MG_DEBUG(("%lu chunk: %lu/%lu, %lu/%lu, ok: %d", c->id, aligned,
              c->recv.len, us->received, us->expected, ok));
    mg_iobuf_del(&c->recv, 0, aligned);  // Delete received data
    if (ok == false) {
      mg_http_reply(c, 400, "", "Upload error\n");
      close_uploaded_file(us);
      c->is_draining = 1;  // Close connection when response it sent
    } else if (us->received >= us->expected) {
      // Uploaded everything. Send response back
      MG_INFO(("%lu done, %lu bytes", c->id, us->received));
      mg_http_reply(c, 200, NULL, "%lu ok\n", us->received);
      close_uploaded_file(us);
      c->is_draining = 1;  // Close connection when response it sent
    }
  }

  // Close uploading file descriptor
  if (us->marker == 'U' && ev == MG_EV_CLOSE) close_uploaded_file(us);
  (void) ev_data;
}

static void get_file_name_from_uri(struct mg_str uri, char *buf, size_t len) {
  struct mg_str parts[3];
  memset(parts, 0, sizeof(parts));           // Init match parts
  mg_match(uri, mg_str("/api/*/#"), parts);  // Fetch file name
  mg_url_decode(parts[1].buf, parts[1].len, buf, len, 0);
}

static void prep_upload(struct mg_connection *c, struct mg_http_message *hm,
                        void *(*fn_open)(char *, size_t),
                        bool (*fn_close)(void *),
                        bool (*fn_write)(void *, void *, size_t)) {
  struct upload_state *us = (struct upload_state *) c->data;
  char path[MG_PATH_MAX];
  memset(us, 0, sizeof(*us));  // Cleanup upload state
  get_file_name_from_uri(hm->uri, path, sizeof(path));
  us->fp = fn_open(path, hm->body.len);
  MG_DEBUG(("file: [%s] size: %lu fp: %p", path, hm->body.len, us->fp));
  us->marker = 'U';  // Mark us as an upload connection
  if (us->fp == NULL) {
    mg_http_reply(c, 400, JSON_HEADERS, "File open error\n");
    c->is_draining = 1;
  } else {
    us->expected = hm->body.len;              // Store number of bytes we expect
    us->fn_close = fn_close;                  // Store closing function
    us->fn_write = fn_write;                  // Store writing function
    mg_iobuf_del(&c->recv, 0, hm->head.len);  // Delete HTTP headers
    c->fn = upload_handler;                   // Change event handler function
    c->pfn = NULL;                            // Detach HTTP handler
    mg_call(c, MG_EV_READ, &c->recv.len);     // Write initial data
  }
}

static void handle_uploads(struct mg_connection *c, int ev, void *ev_data) {
  struct upload_state *us = (struct upload_state *) c->data;
  struct mg_http_message *hm = (struct mg_http_message *) ev_data;

  // Catch /upload requests early, without buffering whole body
  // When we receive MG_EV_HTTP_HDRS event, that means we've received all
  // HTTP headers but not necessarily full HTTP body
  if (ev == MG_EV_HTTP_HDRS && us->marker == 0 &&
      mg_strcmp(hm->method, mg_str("POST")) == 0) {
    struct apihandler *h = find_handler(hm);
    if (h != NULL &&
        (strcmp(h->type, "upload") == 0 || strcmp(h->type, "ota") == 0)) {
      struct apihandler_upload *hu = (struct apihandler_upload *) h;
      prep_upload(c, hm, hu->opener, hu->closer, hu->writer);
    } else if (h != NULL && strcmp(h->type, "file") == 0) {
      struct apihandler_file *hf = (struct apihandler_file *) h;
      prep_upload(c, hm, hf->opener, file_closer, file_writer);
    }
  }
}

static void handle_action(struct mg_connection *c, struct mg_http_message *hm,
                          bool (*check_fn)(void),
                          void (*start_fn)(struct mg_str)) {
  if (hm->body.len > 0) {
    start_fn(hm->body);
    if (check_fn()) {
      struct action_state *as = (struct action_state *) c->data;
      as->marker = 'A';
      as->fn = check_fn;
    } else {
      mg_http_reply(c, 200, JSON_HEADERS, "false");
    }
  } else {
    mg_http_reply(c, 200, JSON_HEADERS, "%s", check_fn() ? "true" : "false");
  }
}

size_t print_struct(void (*out)(char, void *), void *ptr, va_list *ap) {
  const struct attribute *a = va_arg(*ap, struct attribute *);
  char *data = va_arg(*ap, char *);
  size_t i, len = 0;
  for (i = 0; a[i].name != NULL; i++) {
    char *attrptr = data + a[i].offset;
    len += mg_xprintf(out, ptr, "%s%m:", i == 0 ? "" : ",", MG_ESC(a[i].name));
    if (strcmp(a[i].type, "int") == 0) {
      len += mg_xprintf(out, ptr, "%d", *(int *) attrptr);
    } else if (strcmp(a[i].type, "double") == 0) {
      const char *fmt = a[i].format;
      if (fmt == NULL) fmt = "%g";
      len += mg_xprintf(out, ptr, fmt, *(double *) attrptr);
    } else if (strcmp(a[i].type, "bool") == 0) {
      len += mg_xprintf(out, ptr, "%s", *(bool *) attrptr ? "true" : "false");
    } else if (strcmp(a[i].type, "string") == 0) {
      len += mg_xprintf(out, ptr, "%m", MG_ESC(attrptr));
    } else {
      len += mg_xprintf(out, ptr, "null");
    }
  }
  return len;
}

static void handle_object(struct mg_connection *c, struct mg_http_message *hm,
                          struct apihandler_data *h) {
  void *data = calloc(1, h->data_size);
  h->getter(data);
  if (hm->body.len > 0 && h->data_size > 0) {
    char *tmp = calloc(1, h->data_size);
    size_t i;
    memcpy(tmp, data, h->data_size);
    for (i = 0; h->attributes[i].name != NULL; i++) {
      const struct attribute *a = &h->attributes[i];
      char jpath[100];
      mg_snprintf(jpath, sizeof(jpath), "$.%s", a->name);
      if (strcmp(a->type, "int") == 0) {
        double d;
        if (mg_json_get_num(hm->body, jpath, &d)) {
          int v = (int) d;
          memcpy(tmp + a->offset, &v, sizeof(v));
        }
      } else if (strcmp(a->type, "bool") == 0) {
        mg_json_get_bool(hm->body, jpath, (bool *) (tmp + a->offset));
      } else if (strcmp(a->type, "double") == 0) {
        mg_json_get_num(hm->body, jpath, (double *) (tmp + a->offset));
      } else if (strcmp(a->type, "string") == 0) {
        mg_json_get_str2(hm->body, jpath, tmp + a->offset, a->size);
      }
    }
    // If structure changes, increment version
    if (memcmp(data, tmp, h->data_size) != 0) s_device_change_version++;
    if (h->setter != NULL) h->setter(tmp);  // Can be NULL if readonly
    free(tmp);
    h->getter(data);  // Re-sync again after setting
  }
  mg_http_reply(c, 200, JSON_HEADERS, "{%M}\n", print_struct, h->attributes,
                data);
  free(data);
}

static size_t print_array(void (*out)(char, void *), void *ptr, va_list *ap) {
  struct apihandler_array *ha = va_arg(*ap, struct apihandler_array *);
  uint64_t size = *va_arg(*ap, uint64_t *);
  uint64_t start = *va_arg(*ap, uint64_t *);
  size_t i, max = 20, len = 0;
  void *data = calloc(1, ha->data_size);
  for (i = 0; i < max && start + i < size; i++) {
    ha->getter(start + i, data);
    if (i > 0) len += mg_xprintf(out, ptr, ",");
    len += mg_xprintf(out, ptr, "{%M}", print_struct, ha->attributes, data);
  }
  free(data);
  return len;
}

static void handle_array(struct mg_connection *c, struct mg_http_message *hm,
                         struct apihandler_array *h) {
  char buf[40] = "";
  uint64_t size = h->sizer();
  uint64_t start = 0;
  mg_http_get_var(&hm->query, "start", buf, sizeof(buf));
  if (!mg_str_to_num(mg_str(buf), 10, &start, sizeof(start))) start = 0;
  mg_http_reply(c, 200, JSON_HEADERS, "{%m:%llu, %m:%llu, %m:[%M]}\n",
                MG_ESC("size"), size, MG_ESC("start"), start, MG_ESC("data"),
                print_array, h, &size, &start);
}

size_t print_timeseries(void (*out)(char, void *), void *ptr, va_list *ap) {
  uint32_t *timestamps = va_arg(*ap, uint32_t *);
  double *values = va_arg(*ap, double *);
  size_t count = va_arg(*ap, size_t);
  size_t i, len = 0;
  for (i = 0; i < count; i++) {
    const char *comma = i == 0 ? "" : ",";
    len += mg_xprintf(out, ptr, "%s[%lu,%g]", comma, timestamps[i], values[i]);
  }
  return len;
}

static void handle_file(struct mg_connection *c, struct mg_http_message *hm,
                        struct apihandler_file *hf) {
  char path[MG_PATH_MAX];
  get_file_name_from_uri(hm->uri, path, sizeof(path));
  hf->server(c, hm, path);
}

static void handle_api_call(struct mg_connection *c, struct mg_http_message *hm,
                            struct apihandler *h) {
  if (strcmp(h->type, "object") == 0 || strcmp(h->type, "data") == 0) {
    handle_object(c, hm, (struct apihandler_data *) h);
  } else if (strcmp(h->type, "array") == 0) {
    handle_array(c, hm, (struct apihandler_array *) h);
  } else if (strcmp(h->type, "action") == 0) {
    struct apihandler_action *ha = (struct apihandler_action *) h;
    handle_action(c, hm, ha->checker, ha->starter);
  } else if (strcmp(h->type, "file") == 0) {
    handle_file(c, hm, (struct apihandler_file *) h);
  } else if (strcmp(h->type, "custom") == 0) {
    ((struct apihandler_custom *) h)->reply(c, hm);
  } else {
    mg_http_reply(c, 500, JSON_HEADERS, "API type %s unknown\n", h->type);
  }
}

void glue_update_state(void) {
  s_device_change_version++;
}

// Mongoose event handler function, gets called by the mg_mgr_poll()
static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_HDRS && c->data[0] == 0) {
#if WIZARD_ENABLE_HTTP_UI_LOGIN
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (mg_match(hm->uri, mg_str("/api/#"), NULL) ||
        mg_match(hm->uri, mg_str("/websocket"), NULL)) {
      struct apihandler *h = find_handler(hm);
      struct user *u = authenticate(hm);
      if ((u == NULL ||
           (h != NULL && (u->level < h->read_level ||
                          (hm->body.len > 0 && u->level < h->write_level))))) {
        mg_http_reply(c, 403, JSON_HEADERS, "Not Authorised\n");
        c->data[0] = 'Z';  // Mark this connection as handled
      }
    }
#endif
  }

  // We're checking c->is_websocket cause WS connection use c->data
  if (c->is_websocket == 0) handle_uploads(c, ev, ev_data);
  if (ev == MG_EV_POLL && c->is_websocket == 0 && c->data[0] == 'A') {
    // Check if action in progress is complete
    struct action_state *as = (struct action_state *) c->data;
    if (as->fn() == false) {
      mg_http_reply(c, 200, JSON_HEADERS, "true");
      memset(as, 0, sizeof(*as));
    }
  } else if (ev == MG_EV_HTTP_MSG && c->is_websocket == 0 && c->data[0] == 0) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
#if WIZARD_ENABLE_HTTP || WIZARD_ENABLE_HTTPS
    struct apihandler *h = find_handler(hm);
#if WIZARD_ENABLE_HTTP_UI_LOGIN
    struct user *u = authenticate(hm);
    if (mg_match(hm->uri, mg_str("/api/login"), NULL)) {
      handle_login(c, u);
    } else if (mg_match(hm->uri, mg_str("/api/logout"), NULL)) {
      handle_logout(c);
    } else
#endif
        if (mg_match(hm->uri, mg_str("/api/ok"), NULL)) {
      mg_http_reply(c, 200, JSON_HEADERS, "true\n");
    } else if (mg_match(hm->uri, mg_str("/websocket"), NULL)) {
      mg_ws_upgrade(c, hm, NULL);
    } else if (mg_match(hm->uri, mg_str("/api/heartbeat"), NULL)) {
      mg_http_reply(c, 200, JSON_HEADERS, "{%m:%lu}\n", MG_ESC("version"),
                    s_device_change_version);
    } else if (h != NULL) {
      handle_api_call(c, hm, h);
    } else if (c->data[0] == 0)
#endif  // WIZARD_ENABLE_HTTP || WIZARD_ENABLE_HTTPS
    {
#if WIZARD_ENABLE_HTTP_UI
      struct mg_http_serve_opts opts;
      memset(&opts, 0, sizeof(opts));
      opts.root_dir = "/web_root/";
      opts.fs = &mg_fs_packed;
      opts.extra_headers = NO_CACHE_HEADERS;
      mg_http_serve_dir(c, hm, &opts);
#else
      mg_http_reply(c, 200, "", ":)\n");
#endif  // WIZARD_ENABLE_HTTP_UI
    }
  } else if (ev == MG_EV_WS_MSG || ev == MG_EV_WS_CTL) {
    // Ignore received data
  } else if (ev == MG_EV_ACCEPT) {
    if (c->fn_data != NULL) {  // TLS listener
      struct mg_tls_opts opts;
      memset(&opts, 0, sizeof(opts));
      opts.cert = mg_str(TLS_CERT);
      opts.key = mg_str(TLS_KEY);
      mg_tls_init(c, &opts);
    }
  }

  if (ev == MG_EV_HTTP_MSG) {
    // Show this request
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    MG_DEBUG(("%lu %.*s %.*s %lu -> %.*s %lu", c->id, hm->method.len,
              hm->method.buf, hm->uri.len, hm->uri.buf, hm->body.len,
              c->send.len > 15 ? 3 : 0, &c->send.buf[9], c->send.len));
    if (c->data[0] == 'Z') {
      c->data[0] = 0;
      c->is_resp = 0;
    }
  }
}

#if WIZARD_ENABLE_WEBSOCKET
struct ws_handler {
  unsigned timeout_ms;
  void (*fn)(struct mg_connection *);
};
static struct ws_handler
    s_ws_handlers[sizeof(((struct mg_connection *) 0)->data) /
                  sizeof(struct ws_handler)];
static size_t s_ws_handlers_count;

void mongoose_add_ws_handler(unsigned ms, void (*fn)(struct mg_connection *)) {
  size_t max = sizeof(s_ws_handlers) / sizeof(s_ws_handlers[0]);
  if (s_ws_handlers_count >= max) {
    MG_ERROR(("WS handlers limit exceeded, max %lu", max));
  } else {
    s_ws_handlers[s_ws_handlers_count].timeout_ms = ms;
    s_ws_handlers[s_ws_handlers_count].fn = fn;
    s_ws_handlers_count++;
  }
};

static void send_websocket_data(void) {
  struct mg_connection *c;
  uint64_t now = mg_millis();

  for (c = g_mgr.conns; c != NULL; c = c->next) {
    uint64_t *timers = (uint64_t *) &c->data[0];
    size_t i;

    if (c->is_websocket == 0) continue;  // Not a websocket connection? Skip
    if (c->send.len > 2048) continue;    // Too much data already? Skip

    for (i = 0; i < s_ws_handlers_count; i++) {
      if (c->pfn_data == NULL ||
          mg_timer_expired(&timers[i], s_ws_handlers[i].timeout_ms, now)) {
        s_ws_handlers[i].fn(c);
        c->pfn_data = (void *) 1;
      }
    }
  }
}
#else
void mongoose_add_ws_handler(unsigned ms, void (*fn)(struct mg_connection *)) {
  (void) ms, (void) fn;
  MG_ERROR(("Websocket support is not enabled!"));
}
#endif  // WIZARD_ENABLE_WEBSOCKET

#endif  // WIZARD_ENABLE_HTTP || WIZARD_ENABLE_HTTPS

#if WIZARD_ENABLE_SNTP
static uint64_t s_sntp_timer = 0;
bool s_sntp_response_received = false;
static void sntp_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_SNTP_TIME) {
    uint64_t t = *(uint64_t *) ev_data;
    glue_sntp_on_time(t);
    s_sntp_response_received = true;
    s_sntp_timer += (WIZARD_SNTP_INTERVAL_SECONDS) * 1000;
  }
  (void) c;
}

static void sntp_timer(void *param) {
  // uint64_t t1 = mg_now(), t2 = mg_millis();
  uint64_t timeout = (WIZARD_SNTP_INTERVAL_SECONDS) * 1000;
  if (s_sntp_response_received == false) timeout = 1000;
  // This function is called every second. Once we received a response,
  // trigger SNTP sync less frequently, as set by the define
  if (mg_timer_expired(&s_sntp_timer, timeout, mg_millis())) {
    mg_sntp_connect(param, WIZARD_SNTP_URL, sntp_ev_handler, NULL);
  }
}
#endif  // WIZARD_ENABLE_SNTP

#if WIZARD_ENABLE_MQTT
static struct mongoose_mqtt_handlers s_mqtt_handlers = {
    glue_mqtt_connect, glue_mqtt_tls_init, glue_mqtt_on_connect,
    glue_mqtt_on_message, glue_mqtt_on_cmd};

struct mg_connection *g_mqtt_conn;  // MQTT client connection

static void mqtt_event_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_CONNECT) {
    s_mqtt_handlers.tls_init_fn(c);
  } else if (ev == MG_EV_MQTT_OPEN) {
    s_mqtt_handlers.on_connect_fn(c, *(int *) ev_data);
  } else if (ev == MG_EV_MQTT_CMD) {
    s_mqtt_handlers.on_cmd_fn(c, ev_data);
  } else if (ev == MG_EV_MQTT_MSG) {
    struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
    s_mqtt_handlers.on_message_fn(c, mm->topic, mm->data);
  } else if (ev == MG_EV_CLOSE) {
    g_mqtt_conn = NULL;
  }
}

static void mqtt_timer(void *arg) {
  if (g_mqtt_conn == NULL) {
    g_mqtt_conn = s_mqtt_handlers.connect_fn(mqtt_event_handler);
  }
  (void) arg;
}

void mongoose_set_mqtt_handlers(struct mongoose_mqtt_handlers *h) {
  if (h->connect_fn) s_mqtt_handlers.connect_fn = h->connect_fn;
  if (h->tls_init_fn) s_mqtt_handlers.tls_init_fn = h->tls_init_fn;
  if (h->on_message_fn) s_mqtt_handlers.on_message_fn = h->on_message_fn;
  if (h->on_connect_fn) s_mqtt_handlers.on_connect_fn = h->on_connect_fn;
  if (h->on_cmd_fn) s_mqtt_handlers.on_cmd_fn = h->on_cmd_fn;
}
#endif  // WIZARD_ENABLE_MQTT

#if WIZARD_ENABLE_MODBUS
static struct mongoose_modbus_handlers s_modbus_handlers = {
    glue_modbus_read_reg, glue_modbus_write_reg};

static void handle_modbus_pdu(struct mg_connection *c, uint8_t *buf,
                              size_t len) {
  MG_DEBUG(("Received PDU %p len %lu, hexdump:", buf, len));
  mg_hexdump(buf, len);
  // size_t hdr_size = 8, max_data_size = sizeof(response) - hdr_size;
  if (len < 12) {
    MG_ERROR(("PDU too small"));
  } else {
    uint8_t func = buf[7];  // Function
    bool success = false;
    size_t response_len = 0;
    uint8_t response[260];
    memcpy(response, buf, 8);
    // uint16_t tid = mg_ntohs(*(uint16_t *) &buf[0]);  // Transaction ID
    // uint16_t pid = mg_ntohs(*(uint16_t *) &buf[0]);  // Protocol ID
    // uint16_t len = mg_ntohs(*(uint16_t *) &buf[4]);  // PDU length
    // uint8_t uid = buf[6];                            // Unit identifier
    if (func == 6) {  // write single holding register
      uint16_t start = mg_ntohs(*(uint16_t *) &buf[8]);
      uint16_t value = mg_ntohs(*(uint16_t *) &buf[10]);
      success = s_modbus_handlers.write_reg_fn(start, value);
      *(uint16_t *) &response[8] = mg_htons(start);
      *(uint16_t *) &response[10] = mg_htons(value);
      response_len = 12;
      MG_DEBUG(("Glue returned %s", success ? "success" : "failure"));
    } else if (func == 16) {  // Write multiple
      uint16_t start = mg_ntohs(*(uint16_t *) &buf[8]);
      uint16_t num = mg_ntohs(*(uint16_t *) &buf[10]);
      uint16_t i, *data = (uint16_t *) &buf[13];
      if ((size_t) (num * 2 + 10) < sizeof(response)) {
        for (i = 0; i < num; i++) {
          success = s_modbus_handlers.write_reg_fn((uint16_t) (start + i),
                                                   mg_htons(data[i]));
          if (success == false) break;
        }
        *(uint16_t *) &response[8] = mg_htons(start);
        *(uint16_t *) &response[10] = mg_htons(num);
        response_len = 12;
        MG_DEBUG(("Glue returned %s", success ? "success" : "failure"));
      }
    } else if (func == 3 || func == 4) {  // Read multiple
      uint16_t start = mg_ntohs(*(uint16_t *) &buf[8]);
      uint16_t num = mg_ntohs(*(uint16_t *) &buf[10]);
      if ((size_t) (num * 2 + 9) < sizeof(response)) {
        uint16_t i, val, *data = (uint16_t *) &response[9];
        for (i = 0; i < num; i++) {
          success = s_modbus_handlers.read_reg_fn((uint16_t) (start + i), &val);
          if (success == false) break;
          data[i] = mg_htons(val);
        }
        response[8] = (uint8_t) (num * 2);
        response_len = 9 + response[8];
        MG_DEBUG(("Glue returned %s", success ? "success" : "failure"));
      }
    }
    if (success == false) {
      response_len = 9;
      response[7] |= 0x80;
      response[8] = 4;  // Server Device Failure
    }
    *(uint16_t *) &response[4] = mg_htons((uint16_t) (response_len - 6));
    MG_DEBUG(("Sending PDU response %lu:", response_len));
    mg_hexdump(response, response_len);
    mg_send(c, response, response_len);
  }
}

static void modbus_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  // if (ev == MG_EV_OPEN) c->is_hexdumping = 1;
  if (ev == MG_EV_READ) {
    uint16_t len;
    if (c->recv.len < 7) return;  // Less than minimum length, buffer more
    len = mg_ntohs(*(uint16_t *) &c->recv.buf[4]);  // PDU length
    MG_INFO(("Got %lu, expecting %lu", c->recv.len, len + 6));
    if (c->recv.len < len + 6U) return;          // Partial frame, buffer more
    handle_modbus_pdu(c, c->recv.buf, len + 6);  // Parse PDU and call user
    mg_iobuf_del(&c->recv, 0, len + 6U);         // Delete received PDU
  }
  (void) ev_data;
}

void mongoose_set_modbus_handlers(struct mongoose_modbus_handlers *h) {
  if (h->read_reg_fn) s_modbus_handlers.read_reg_fn = h->read_reg_fn;
  if (h->write_reg_fn) s_modbus_handlers.write_reg_fn = h->write_reg_fn;
}
#endif  // WIZARD_ENABLE_MODBUS

#if WIZARD_CAPTIVE_PORTAL

#if MG_ARCH == MG_ARCH_UNIX || MG_ARCH == MG_ARCH_WIN32
#define CAPTIVE_PORTAL_URL "udp://0.0.0.0:5533"
#else
#define CAPTIVE_PORTAL_URL "udp://0.0.0.0:53"
#endif

static const uint8_t answer[] = {
    0xc0, 0x0c,          // Point to the name in the DNS question
    0,    1,             // 2 bytes - record type, A
    0,    1,             // 2 bytes - address class, INET
    0,    0,    0, 120,  // 4 bytes - TTL
    0,    4              // 2 bytes - address length
};

static void dns_fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_READ) {
    struct mg_dns_rr rr;  // Parse first question, offset 12 is header size
    size_t n = mg_dns_parse_rr(c->recv.buf, c->recv.len, 12, true, &rr);
    MG_DEBUG(("DNS request parsed, result=%d", (int) n));
    if (n > 0) {
      char buf[512];
      uint32_t ip;
      struct mg_dns_header *h = (struct mg_dns_header *) buf;
      memset(buf, 0, sizeof(buf));  // Clear the whole datagram
      h->txnid = ((struct mg_dns_header *) c->recv.buf)->txnid;  // Copy tnxid
      h->num_questions = mg_htons(1);  // We use only the 1st question
      h->num_answers = mg_htons(1);    // And only one answer
      h->flags = mg_htons(0x8400);     // Authoritative response
      memcpy(buf + sizeof(*h), c->recv.buf + sizeof(*h), n);  // Copy question
      memcpy(buf + sizeof(*h) + n, answer, sizeof(answer));   // And answer
#if MG_ENABLE_TCPIP
      ip = c->mgr->ifp->ip;
#else
      ip = MG_TCPIP_IP;
#endif
      memcpy(buf + sizeof(*h) + n + sizeof(answer), &ip, 4);
      mg_send(c, buf, 12 + n + sizeof(answer) + 4);  // And send it!
    }
    mg_iobuf_del(&c->recv, 0, c->recv.len);
  }
  (void) ev_data;
}
#endif  // WIZARD_CAPTIVE_PORTAL

void mongoose_init(void) {
  mg_mgr_init(&g_mgr);      // Initialise event manager
  mg_log_set(MG_LL_DEBUG);  // Set log level to debug

#if WIZARD_ENABLE_HTTP
  MG_INFO(("Starting HTTP listener"));
  mg_http_listen(&g_mgr, HTTP_URL, http_ev_handler, NULL);
#endif
#if WIZARD_ENABLE_HTTPS
  MG_INFO(("Starting HTTPS listener"));
  mg_http_listen(&g_mgr, HTTPS_URL, http_ev_handler, "");
#endif

#if WIZARD_ENABLE_SNTP
  MG_INFO(("Starting SNTP timer"));
  mg_timer_add(&g_mgr, 1000, MG_TIMER_REPEAT, sntp_timer, &g_mgr);
#endif

#if WIZARD_DNS_TYPE == 2
  g_mgr.dns4.url = WIZARD_DNS_URL;
#endif

#if WIZARD_ENABLE_MQTT
  MG_INFO(("Starting MQTT reconnection timer"));
  mg_timer_add(&g_mgr, 1000, MG_TIMER_REPEAT, mqtt_timer, &g_mgr);
#endif

#if WIZARD_ENABLE_MODBUS
  {
    char url[100];
    mg_snprintf(url, sizeof(url), "tcp://0.0.0.0:%d", WIZARD_MODBUS_PORT);
    MG_INFO(("Starting Modbus-TCP server on port %d", WIZARD_MODBUS_PORT));
    mg_listen(&g_mgr, url, modbus_ev_handler, NULL);
  }
#endif

#if WIZARD_CAPTIVE_PORTAL
  MG_INFO(("Starting captive portal"));
  mg_listen(&g_mgr, CAPTIVE_PORTAL_URL, dns_fn, NULL);
#endif

#if WIZARD_ENABLE_MDNS
  MG_INFO(("Starting MDNS (domain name: %s.local)", WIZARD_MDNS_NAME));
  mg_mdns_listen(&g_mgr, WIZARD_MDNS_NAME);
#endif

  glue_lock_init();
  MG_INFO(("Mongoose init complete"));
}

void mongoose_poll(void) {
  glue_lock();
  mg_mgr_poll(&g_mgr, 10);
#if WIZARD_ENABLE_WEBSOCKET
  send_websocket_data();
#endif
  glue_unlock();
}