# Introduction

![pjsip layout](http://www.pjsip.org/docs/latest/pjsip/docs/html/pjsip-arch.jpg)

Enumerating the static libraries from the bottom:

* PJLIB, is the platform abstraction and framework library, on which all other libraries depend,

* PJLIB-UTIL, provides auxiliary functions such as text scanning, XML, and STUN,

* PJMEDIA is the multimedia framework,

* PJMEDIA-CODEC is the placeholder for media codecs,
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
 * Faster copy operations

* Disadvantages
 * A `pj_str_t` struct doesn't "own" its `ptr` i.e. it is not responsible for managing the memory of `ptr`. So memory management is a little tricker.

---

# Strings -- Pop Quiz


---

# Inheritance in pjlib

* Inheritance using `struct`s

* A `struct` should begin with its super's fields, followed by its own

* Base class

        struct pjsip_uri
        {
            pjsip_uri_vptr *vptr;		/**< Pointer to virtual function table.*/
        };

* Sub class

        typedef struct pjsip_sip_uri
        {
            pjsip_uri_vptr *vptr;		/**< Pointer to virtual function table.*/
            pj_str_t	    user;		/**< Optional user part. */
            pj_str_t	    passwd;		/**< Optional password part. */
            pj_str_t	    host;		/**< Host part, always exists. */
            int		    port;		/**< Optional port number, or zero. */
            ...
        } pjsip_sip_uri;

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

* Create a subclass instance (e.g. `pjsip_sip_uri`)

        pjsip_sip_uri *uri = pjsip_sip_uri_create(pool, 0)

* pjsip_sip_uri_create

        PJ_DEF(pjsip_sip_uri*) pjsip_sip_uri_create( pj_pool_t *pool,
                                                     pj_bool_t secure )
        {
            pjsip_sip_uri *url = PJ_POOL_ALLOC_T(pool, pjsip_sip_uri);
            pjsip_sip_uri_init(url, secure);
            return url;
        }

        PJ_DEF(void) pjsip_sip_uri_init(pjsip_sip_uri *url, pj_bool_t secure)
        {
            pj_bzero(url, sizeof(*url));
            url->ttl_param = -1;
            url->vptr = secure ? &sips_url_vptr : &sip_url_vptr;
            pj_list_init(&url->other_param);
            pj_list_init(&url->header_param);
        }

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

# Using pjsua -- Instantiate pjsua

* Instantiation

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

# Using pjsua -- thread registration

* Thread registration

        static pj_thread_desc aPJThreadDesc; // needs to last for the lifetime of thread
        if (!pj_thread_is_registered()) {
            pj_thread_t *pjThread;
            status = pj_thread_register(NULL, aPJThreadDesc, &pjThread);
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

# Using pjsua -- memory pool

* Creating a memory pool

        pj_pool_t *aPJPool;
        aPJPool = pjsua_pool_create("AKSIPUserAgent-pjsua", 1000, 1000);

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
 * `max_calls`
 * `outbound_proxy_cnt`, `outbound_proxy`
 * `stun_host`
 * `user_agent`
 * `thread_cnt`
 * `cb` (`pjsua_callback`)

---

# pjsua_callback

* What is it?

        /**
         * This structure describes application callback to receive various event
         * notification from PJSUA-API. All of these callbacks are OPTIONAL,
         * although definitely application would want to implement some of
         * the important callbacks (such as \a on_incoming_call).
         */

* if `pjsua_config.thread_cnt` > 0, callbacks will will be invoked on worker threads

* Callbacks
 * `on_reg_state`
 * `on_incoming_call`
 * `on_call_state`
 * `on_call_media_state`

---

# pjsua_logging_config

* What is it?

        /**
         * Logging configuration, which can be (optionally) specified when calling
         * #pjsua_init(). Application must call #pjsua_logging_config_default() to
         * initialize this structure with the default values.
         */

* Fields
 * `level`
 * `console_level`
 * `msg_logging`
 * `log_filename`
 * `cb`

---

# pjsua_media_config

* What is it?

        /**
         * This structure describes media configuration, which will be specified
         * when calling #pjsua_init(). Application MUST initialize this structure
         * by calling #pjsua_media_config_default().
         */

* Fields
 * `no_vad`
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

# Using _config

    pjsua_config ua_config;
    pjsua_config_default(&ua_config); // Always setup default values first!
    ua_config.outbound_proxy_cnt = 1;
    ua_config.outbound_proxy[0] = ...
    ua_config.stun_host = ...

---

# Initialize pjsua

    pj_status_t status = pjsua_init(&userAgentConfig, &loggingConfig, &mediaConfig);
    if (status != PJ_SUCCESS) {
        NSLog(@"Error initializing PJSUA");
        return;
    }

---

# Add transports

* UDP

        // Add UDP transport.
        pjsua_transport_id transportIdentifier;
        status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transportConfig, &transportIdentifier);
        if (status != PJ_SUCCESS) {
            NSLog(@"Error creating transport");
            return;
        }

* TCP

        // Add TCP transport. Don't return, just leave a log message on error.
        status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &transportConfig, NULL);
        if (status != PJ_SUCCESS) {
            NSLog(@"Error creating TCP transport");
        }

---

# Start pjsua

    // Start PJSUA.
    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        NSLog(@"Error starting PJSUA");
        return;
    }