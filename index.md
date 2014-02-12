# Introduction

![pjsip layout](http://www.pjsip.org/docs/latest/pjsip/docs/html/pjsip-arch.jpg)

Enumerating the static libraries from the bottom:

* PJLIB is the platform abstraction and framework library on which all other libraries depend

* PJLIB-UTIL provides auxiliary functions such as text scanning, XML, and STUN

* PJMEDIA is the multimedia framework

* PJMEDIA-CODEC is the placeholder for media codecs

* Core SIP Library (PJSIP-CORE) is the very core of the PJSIP library, and contains the SIP Endpoint, which is the owner/manager for all SIP objects in the application, messaging elements, parsing, transport management, module management, and stateless operations, and a user agent library

* PJSUA API - High Level Softphone API (PJSUA-LIB) is the highest level of abstraction, which wraps together all above functionalities into high level, easy to use API.

Ref: http://www.pjsip.org/docs/latest/pjsip/docs/html/index.htm

---

# PJLIB

* Uses a [Memory Pool][] instead of `malloc()`

* Uses [pj_str_t][] instead of `char *`

* Requires app to [register][Thread Registration] external threads with pjlib

* [PJLIB Documentation][]

[pj_str_t]: http://www.pjsip.org/docs/latest/pjlib/docs/html/group__PJ__PSTR.htm
[Memory Pool]: http://www.pjsip.org/docs/latest/pjlib/docs/html/group__PJ__POOL__GROUP.htm
[Thread Registration]: http://www.pjsip.org/docs/latest/pjlib/docs/html/group__PJ__THREAD.htm#ga600d2f8baddfd78de3b6b60cce027a9a
[PJLIB Documentation]: http://www.pjsip.org/docs/latest/pjlib/docs/html/index.htm

---

# Memory Pool

* For allocation of objects, consider using pjlib's memory pool instead of malloc/free

* Used extensively by all of pjsip

* Faster than malloc

* Not thread safe

* Order of operations
 * Create pool factory (typically only once per application)
 * Create the pool
 * Allocate memory as required
 * Destroy the pool
 * Destroy the pool factory

* No way to release chunks back to memory pools; chunks are released when the pool itself is destroyed

* A pool's size never shrinks, it only grows until it is destroyed

* When allocating a chunk, consider carefully the lifetime of the pool
 * e.g. if the pool's lifetime is the same as application's, the chunk's memory will never get released

---

# Memory Pool

* Create pool factory

        pj_caching_pool cp;
        pj_caching_pool_init(&cp, NULL, 1024 * 1024);

* Create the pool

        pj_pool_t *pool = pj_pool_create(&cp.factory, "pool1", 4000, 4000, NULL);

* Allocate memory as required

        pjsip_sip_uri *uri = pj_pool_alloc(pool, sizeof(pjsip_sip_uri));

* Destroy the pool

        pj_pool_release(pool);

* Destroy the pool factory

        pj_caching_pool_destroy(&cp);

---

# Strings

* `pj_str_t`

         typedef struct pj_str_t
         {
             char      *ptr;
             pj_size_t  slen;
         } pj_str_t;

* `ptr` is _not_ NULL terminated

* Advantages
 * Faster, more memory efficient substring operations -- because strings don't need to be NULL terminated, substrings can re-use same storage as the original string
 * More efficient copy operations

* Disadvantages
 * A `pj_str_t` struct doesn't "own" its `ptr` i.e. it is not responsible for managing the memory of `ptr`. So memory management is a little tricker.

---

# Strings -- Functions

* `pj_str` -- Create a `pj_str_t` from a normal C string (C string is not copied)

        pj_str("Hello");

* `pj_cstr` -- Return a constant `pj_str_t` from a normal C string (C string is not copied)

          return pj_cstr(pj_pool_alloc(pool, sizeof(pj_str_t)), "Hello");

* `pj_strset` -- Set pointer and length to specified value

        return pj_strset(pj_pool_alloc(pool, sizeof(pj_str_t)), "Hello", 5);

* `pj_strassign` -- Assign `ptr` and `slen` from one `pj_str_t` to another (the actual C string is not copied)

* `pj_strcpy`, `pj_strcpy2`, `pj_strncpy` -- Copy one `pj_str_t` to another (the C string IS copied, but destination is assumed to have already allocated the required space for the C string)

* `pj_strdup`, `pj_strdup2`, `pj_strdup_with_null`, `pj_strdup3` -- Copy one `pj_str_t` to another (the C string is copied and space for destination string is allocated from the given pool)

* `pj_strlen`, `pj_strbuf` -- get the C string pointer and length of string

---

# Strings -- Pop Quiz

Will you face any problems related to memory?

```cpp
pj_str_t test1() {
  return pj_str("Hello");
}

pj_str_t test2() {
  char *str = "Hello";
  return pj_str(str);
}

pj_str_t test3() {
  char str[] = "Hello";
  return pj_str(str);
}

const pj_str_t *test4() {
  pj_str_t pstr = pj_str("Hello");
  return &pstr;
}
```

---

# Strings -- Pop Quiz

Will you face any problems related to memory?

```cpp
const pj_str_t *test5() {
  return pj_cstr(malloc(sizeof(pj_str_t)), "Hello");
}

const pj_str_t *test6(pj_pool_t *pool) {
  return pj_cstr(pj_pool_alloc(pool, sizeof(pj_str_t)), "Hello");
}

pj_str_t test7() {
  char original[] = "Hello";
  char *dup = malloc(sizeof(original));
  strncpy(dup, original, sizeof(original));
  return pj_str(dup);
}

pj_str_t test8(pj_pool_t *pool) {
  char original[] = "Hello";
  char *dup = pj_pool_alloc(pool, sizeof(original));
  strncpy(dup, original, sizeof(original));
  return pj_str(dup);
}
```

---

# Inheritance

* Inheritance is achieved in all pjsip modules using `struct`s

* A `struct` should begin with its super's fields, followed by its own

* Base "class"

        struct pjsip_uri
        {
            pjsip_uri_vptr *vptr;		/**< Pointer to virtual function table.*/
        };

* "Subclass"

        typedef struct pjsip_sip_uri
        {
            pjsip_uri_vptr *vptr;		/**< Pointer to virtual function table.*/
            pj_str_t	    user;		/**< Optional user part. */
            pj_str_t	    passwd;		/**< Optional password part. */
            pj_str_t	    host;		/**< Host part, always exists. */
            int		    port;		/**< Optional port number, or zero. */
            ...
        } pjsip_sip_uri;

* Functions written for base class work on subclasses too

        PJ_INLINE(void*) pjsip_uri_clone( pj_pool_t *pool, const void *uri )
        {
            return (*((const pjsip_uri*)uri)->vptr->p_clone)(pool, uri);
        }

        pjsip_sip_uri *uri = ...
        pjsip_sip_uri *new_uri = (pjsip_sip_uri *)pjsip_uri_clone(pool, (pjsip_sip_uri *)uri);

---

# Polymorphism in pjlib

* Virtual function tables

* pjsip_uri_vptr

        typedef struct pjsip_uri_vptr
        {
            const pj_str_t* (*p_get_scheme)(const void *uri);

            void* (*p_get_uri)(void *uri);

            pj_ssize_t (*p_print)(pjsip_uri_context_e context,
                                  const void *uri,
                                  char *buf, pj_size_t size);

            pj_status_t	(*p_compare)(pjsip_uri_context_e context,
                                     const void *uri1, const void *uri2);

            void *(*p_clone)(pj_pool_t *pool, const void *uri);
        } pjsip_uri_vptr;

* Generic functions on `pjsip_uri`

        PJ_INLINE(const pj_str_t*) pjsip_uri_get_scheme(const void *uri)
        {
            return (*((pjsip_uri*)uri)->vptr->p_get_scheme)(uri);
        }

        PJ_INLINE(void*) pjsip_uri_get_uri(const void *uri)
        {
            return (*((pjsip_uri*)uri)->vptr->p_get_uri)((void*)uri);
        }

        ...

---

# Polymorphism in pjlib

* Putting it all together

* To create a subclass of `pjsip_uri`, define its virtual function table

        static pjsip_uri_vptr sip_url_vptr =
        {
            (P_GET_SCHEME)	&pjsip_url_get_scheme,
            (P_GET_URI)		&pjsip_get_uri,
            (P_PRINT_URI) 	&pjsip_url_print,
            (P_CMP_URI) 	&pjsip_url_compare,
            (P_CLONE) 		&pjsip_url_clone
        };

* Provide constructors for the subclass

        PJ_DEF(pjsip_sip_uri*) pjsip_sip_uri_create( pj_pool_t *pool,
                                                     pj_bool_t secure )
        {
            pjsip_sip_uri *url = PJ_POOL_ALLOC_T(pool, pjsip_sip_uri);
            pj_bzero(url, sizeof(*url));
            url->ttl_param = -1;
            url->vptr = secure ? &sips_url_vptr : &sip_url_vptr;
            pj_list_init(&url->other_param);
            pj_list_init(&url->header_param);
            return url;
        }

* Using a subclass

        pjsip_sip_uri *uri = pjsip_sip_uri_create(pool, 0);

---

# Linked lists in pjsip

* `pj_list` is the "base" class for many structures in pjsip

* Avoids dynamic memory allocation when adding nodes to a list, but has other limitations (you can't put a node in more than one list)

* Definition

        /**
         * Use this macro in the start of the structure declaration to declare that
         * the structure can be used in the linked list operation. This macro simply
         * declares additional member @a prev and @a next to the structure.
         * @hideinitializer
         */
        #define PJ_DECL_LIST_MEMBER(type)                       \
                                           /** List @a prev. */ \
                                           type *prev;          \
                                           /** List @a next. */ \
                                           type *next

        /**
         * This structure describes generic list node and list. The owner of this list
         * must initialize the 'value' member to an appropriate value (typically the
         * owner itself).
         */
        struct pj_list
        {
            PJ_DECL_LIST_MEMBER(void);
        } PJ_ATTR_MAY_ALIAS; /* may_alias avoids warning with gcc-4.4 -Wall -O2 */

---

# Linked lists in pjsip

* Some operations on linked lists

        PJ_INLINE(void) pj_list_init(pj_list_type * node)
        {
            ((pj_list*)node)->next = ((pj_list*)node)->prev = node;
        }

        PJ_INLINE(int) pj_list_empty(const pj_list_type * node)
        {
            return ((pj_list*)node)->next == node;
        }

        PJ_IDEF(void) pj_list_insert_after(pj_list_type *pos, pj_list_type *node)
        {
            ((pj_list*)node)->prev = pos;
            ((pj_list*)node)->next = ((pj_list*)pos)->next;
            ((pj_list*) ((pj_list*)pos)->next) ->prev = node;
            ((pj_list*)pos)->next = node;
        }

---

# Linked lists in pjsip

* SIP Headers are implemented as linked list nodes -- so any operations on linked lists apply to headers too

* Generic Header

        struct pjsip_hdr
        {
            PJSIP_DECL_HDR_MEMBER(struct pjsip_hdr);
        };

        #define PJSIP_DECL_HDR_MEMBER(hdr)   \
            /** List members. */	\
            PJ_DECL_LIST_MEMBER(hdr);	\
            /** Header type */		\
            pjsip_hdr_e	    type;	\
            /** Header name. */		\
            pj_str_t	    name;	\
            /** Header short name version. */	\
            pj_str_t	    sname;		\
            /** Virtual function table. */	\
            pjsip_hdr_vptr *vptr

* Generic string header

        typedef struct pjsip_generic_string_hdr
        {
            /** Standard header field. */
            PJSIP_DECL_HDR_MEMBER(struct pjsip_generic_string_hdr);
            /** hvalue */
            pj_str_t hvalue;
        } pjsip_generic_string_hdr;

---

# PJSUA

Ships with a [command line app](http://www.pjsip.org/pjsua.htm) for testing and reference.

![pjsua cli screenshot](http://www.pjsip.org/pjsip/docs/html/pjsua.jpg)

---

# PJSUA initialization and shutdown

    int main()
    {
        pj_init();

        // Continue with PJSUA initialization here or create a secondary thread
        // see run_pjsua()
        ....
        // After pjsua_destroy() is called

        pj_shutdown();
    }

    int run_pjsua()
    {
        // Register the thread, after pj_init() is called
        pj_thread_register();

        // Create pjsua and pool
        pjsua_create();
        pjsua_pool_create();

        // Init pjsua
        pjsua_init();

        // Start pjsua
        pjsua_start();

        .........

        // Destroy pjsua
        pjsua_destroy();
    }

Ref: https://trac.pjsip.org/repos/wiki/PJSUA_Initialization

---

# Using pjsua -- Initialize pjlib

* Usage

        pj_status_t status = pj_init();
        if (status != PJ_SUCCESS) {
          NSLog(@"Error initializing pjlib");
          return;
        }

* pj_init

        /**
         * Initialize the PJ Library.
         * This function must be called before using the library. The purpose of this
         * function is to initialize static library data, such as character table used
         * in random string generation, and to initialize operating system dependent
         * functionality (such as WSAStartup() in Windows).
         *
         * Apart from calling pj_init(), application typically should also initialize
         * the random seed by calling pj_srand().
         *
         * @return PJ_SUCCESS on success.
         */
        PJ_DECL(pj_status_t) pj_init(void);

---

# Using pjsua -- thread registration

* Register external threads using `pj_thread_register` before calling anything apart from `pj_init`.

* Usage

        static pj_thread_desc thread_desc; // needs to last for the lifetime of thread
        if (!pj_thread_is_registered()) {
            pj_thread_t *thread_handle;
            status = pj_thread_register(NULL, thread_desc, &thread_handle);
            if (status != PJ_SUCCESS) {
                NSLog(@"Error registering thread at PJSUA");
                return;
            }
        }

* pj_thread_register

        /**
         * Register a thread that was created by external or native API to PJLIB.
         * This function must be called in the context of the thread being registered.
         * When the thread is created by external function or API call,
         * it must be 'registered' to PJLIB using pj_thread_register(), so that it can
         * cooperate with PJLIB's framework. During registration, some data needs to
         * be maintained, and this data must remain available during the thread's
         * lifetime.
         *
         * @param thread_name   The optional name to be assigned to the thread.
         * @param desc          Thread descriptor, which must be available throughout
         *                      the lifetime of the thread.
         * @param thread        Pointer to hold the created thread handle.
         *
         * @return              PJ_SUCCESS on success, or the error code.
         */
        PJ_DECL(pj_status_t) pj_thread_register ( const char *thread_name,
                              pj_thread_desc desc,
                              pj_thread_t **thread);

---

# Using pjsua -- Instantiate pjsua

* Usage

        pj_status_t status = pjsua_create();
        if (status != PJ_SUCCESS) {
          NSLog(@"Error creating PJSUA");
          return;
        }

* pjsua_create

        /**
         * Instantiate pjsua application. Application must call this function before
         * calling any other functions, to make sure that the underlying libraries
         * are properly initialized. Once this function has returned success,
         * application must call pjsua_destroy() before quitting.
         *
         * @return		PJ_SUCCESS on success, or the appropriate error code.
         */
        PJ_DECL(pj_status_t) pjsua_create(void);

---

# Using pjsua -- memory pool

* Usage

        pj_pool_t *pool;
        pool = pjsua_pool_create("pjsua user agent", 1000, 1000);

* pjsua_pool_create

        /**
         * Create memory pool to be used by the application. Once application
         * finished using the pool, it must be released with pj_pool_release().
         *
         * @param name		Optional pool name.
         * @param init_size	Initial size of the pool.
         * @param increment	Increment size.
         *
         * @return		The pool, or NULL when there's no memory.
         */
        PJ_DECL(pj_pool_t*) pjsua_pool_create(const char *name, pj_size_t init_size,
                              pj_size_t increment);

---

# pjsua_config

* What is it?

        /**
         * This structure describes the settings to control the API and
         * user agent behavior, and can be specified when calling #pjsua_init().
         * Before setting the values, application must call #pjsua_config_default()
         * to initialize this structure with the default values.
         */
        typedef struct pjsua_config
        ...

* Important fields
 * `max_calls` -- maximum calls to support at a time
 * `stun_host` -- stun server host
 * `user_agent` -- user agent string
 * `thread_cnt` -- if > 0, callbacks will be invoked on worker threads
 * `cb` -- callbacks

---

# Using pjsua_config

    pjsua_config ua_config;
    pjsua_config_default(&ua_config); // Always setup default values first!
    ua_config.outbound_proxy_cnt = 1;
    ua_config.outbound_proxy[0] = ...
    ua_config.stun_host = ...

---

# pjsua_callback

* What is it?

        /**
         * This structure describes application callback to receive various event
         * notification from PJSUA-API. All of these callbacks are OPTIONAL,
         * although definitely application would want to implement some of
         * the important callbacks (such as \a on_incoming_call).
         */
         typedef struct pjsua_callback
         ...

* Callbacks
 * `on_reg_state` -- changes in account registration state -- succeeded, failed, etc.
 * `on_incoming_call` -- on receiving a call
 * `on_call_state` -- call answered, hung up, etc.
 * `on_call_media_state` -- state of media streams

---

# pjsua_logging_config

* What is it?

        /**
         * Logging configuration, which can be (optionally) specified when calling
         * #pjsua_init(). Application must call #pjsua_logging_config_default() to
         * initialize this structure with the default values.
         */

* Fields
 * `level` -- verbosity level (higher is more verbose)
 * `console_level` -- verbosity level for console
 * `msg_logging` -- should we log SIP messages?
 * `log_filename` -- log filename
 * `cb` -- callback for custom logging

* If neither `log_filename` nor `cb` are provided, logs are writting to stdout.

* Example -- send logs to `stderr`

          // Sometime during setup
          ...
          logging_config.cb = log_writer;
          ...

        static void log_writer(int level, const char *data, int len) {
          fprintf(stderr, "%s", data);
        }

---

# pjsua_media_config

* What is it?

        /**
         * This structure describes media configuration, which will be specified
         * when calling #pjsua_init(). Application MUST initialize this structure
         * by calling #pjsua_media_config_default().
         */

* Fields
 * `no_vad` -- voice activity detection (on by default)
 * `enable_ice`
 * `enable_turn`
 * `turn_server`

---

# pjsua_transport_config

* What is it?

        /**
         * Transport configuration for creating transports for both SIP
         * and media. Before setting some values to this structure, application
         * MUST call #pjsua_transport_config_default() to initialize its
         * values with default settings.
         */

* Fields
 * `port`
 * `bound_addr`
 * `public_addr`

---

# Initialize pjsua

    pj_status_t status = pjsua_init(&ua_config, &logging_config, &media_config);
    if (status != PJ_SUCCESS) {
        NSLog(@"Error initializing PJSUA");
        return;
    }

---

# Add transports

* UDP

        // Add UDP transport.
        pjsua_transport_id transport_id;
        status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transport_config, &transport_id);
        if (status != PJ_SUCCESS) {
            NSLog(@"Error creating transport");
            return;
        }

* TCP

        // Add TCP transport. Don't return, just leave a log message on error.
        status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &transport_config, NULL);
        if (status != PJ_SUCCESS) {
            NSLog(@"Error creating TCP transport");
        }

---

# Start pjsua

    // Start PJSUA.
    pj_status_t status = pjsua_start();
    if (status != PJ_SUCCESS) {
        NSLog(@"Error starting PJSUA");
        return;
    }

# Add an account -- pjsua_account_config

* What is it?

        /**
         * This structure describes account configuration to be specified when
         * adding a new account with #pjsua_acc_add(). Application MUST initialize
         * this structure first by calling #pjsua_acc_config_default().
         */
        typedef struct pjsua_acc_config
        ...

* Important fields
 * `id` -- full SIP URL e.g. "Chaitanya Gupta" <cg@sip.talk.to>
 * `reg_uri` -- URL to be sent for registration e.g. cg@sip.talk.to
 * `cred_count`, `cred_info` -- account credentials
 * `proxy_cnt`, `proxy` -- SIP proxy details
 * `reg_timeout` -- SIP registration timeout
 * `allow_contact_rewrite` -- should be set to true; keeps track of client's public IP address from the response of REGISTER request
 * `sip_stun_use`, `media_stun_use` -- enable or disable STUN for this account
 * `ice_cfg_use`, `ice_cfg` -- ICE configuration for this account
 * `turn_cfg_use`, `turn_cfg` -- TURN configuration for this account
 * `reg_hdr_list` -- send custom headers with REGISTER request

---

# Add an account -- sending a custom header with REGISTER

Assume we want to send an "Auth-Token" header with every REGISTER request

    pj_str_t auth_token_header_name = pj_str("Auth-Token");
    pj_str_t auth_token_header_value = pj_str(auth_token)
    pj_list_push_back(&acc_config.reg_hdr_list, pjsip_generic_string_hdr_create(pool,
                                                                                &auth_token_header_name,
                                                                                &auth_token_header_value));

---

# Add an account

* Usage

        pj_status_t status = pjsua_acc_add(&acc_config, PJ_FALSE,
                                           &acc_id);
        if (status != PJ_SUCCESS) {
          NSLog(@"Error adding account");
          return NO;
        }

* Associate the value in `acc_id` with your representation of a SIP account

---

# Adding an account -- wait for registration to succeed

      // Sometime during setup
      ...
      ua_config.cb.on_reg_state = &sip_reg_state_changed;
      ...

    static void sip_reg_state_changed(pjsua_acc_id acc_id) {
      pjsua_acc_info acc_info;
      pj_status_t status = pjsua_acc_get_info(acc_id, &acc_info);
      if (status != PJ_SUCCESS) {
        NSLog(@"Couldn't get account status");
        return;
      }
      if (acc_info.status == 200) {
        NSLog(@"Account is registered!");
      }
    }

---

# Make a call

* Usage

        pj_str_t dst_uri = pj_str("...")
        pj_call_id call_id;
        pj_status_t status = pjsua_call_make_call(acc_identiifer,
                                                  &dst_uri,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  &call_id);
        if (status != PJ_SUCCESS) {
          NSLog(@"Couldn't make call");
        }

* pjsua_call_make_call

        /**
         * Make outgoing call to the specified URI using the specified account.
         *
         * @param acc_id	The account to be used.
         * @param dst_uri	URI to be put in the To header (normally is the same
         *			as the target URI).
         * @param opt		Optional call setting. This should be initialized
         *			using #pjsua_call_setting_default().
         * @param user_data	Arbitrary user data to be attached to the call, and
         *			can be retrieved later.
         * @param msg_data	Optional headers etc to be added to outgoing INVITE
         *			request, or NULL if no custom header is desired.
         * @param p_call_id	Pointer to receive call identification.
         *
         * @return		PJ_SUCCESS on success, or the appropriate error code.
         */
        PJ_DECL(pj_status_t) pjsua_call_make_call(pjsua_acc_id acc_id,
                                                  const pj_str_t *dst_uri,
                                                  const pjsua_call_setting *opt,
                                                  void *user_data,
                                                  const pjsua_msg_data *msg_data,
                                                  pjsua_call_id *p_call_id);

---

# Monitor call state changes

      // Sometime during setup
      ...
      ua_config.cb.on_call_state = &sip_call_state_changed;
      ...

    static void sip_call_state_changed(pjsua_call_id call_id, pjsip_event *event) {
      pjsua_call_info call_info;
      pj_status_t status = pjsua_call_get_info(call_id, &call_info);
      if (status != PJ_SUCCESS) {
        NSLog(@"Couldn't get call status");
        return;
      }
      if (call_info.state == PJSIP_INV_STATE_CONFIRMED) {
        NSLog(@"call is connected");
      } else if (call_info.state == PJSIP_INV_STATE_DISCONNECTED) {
        NSLog(@"call is disconnected");
      }
    }
