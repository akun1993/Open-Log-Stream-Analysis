.. _thread-safety:

*************
Thread safety
*************

Jansson as a library is thread safe and has no mutable global state.
The only exceptions are the hash function seed and memory allocation
functions, see below.

There's no locking performed inside Jansson's code. **Read-only**
access to JSON values shared by multiple threads is safe, but
**mutating** a JSON value that's shared by multiple threads is not. A
multithreaded program must perform its own locking if JSON values
shared by multiple threads are mutated.

However, **reference count manipulation** (:func:`json_incref()`,
:func:`json_decref()`) is usually thread-safe, and can be performed on
JSON values that are shared among threads. The thread-safety of
reference counting can be checked with the
``JANSSON_THREAD_SAFE_REFCOUNT`` preprocessor constant. Thread-safe
reference count manipulation is achieved using compiler built-in
atomic functions, which are available in most modern compilers.

If compiler support is not available (``JANSSON_THREAD_SAFE_REFCOUNT``
is not defined), it may be very difficult to ensure thread safety of
reference counting. It's possible to have a reference to a value
that's also stored inside an array or object in another thread.
Modifying the container (adding or removing values) may trigger
concurrent access to such values, as containers manage the reference
count of their contained values.


Hash function seed
==================

To prevent an attacker from intentionally causing large JSON objects
with specially crafted keys to perform very slow, the hash function
used by Jansson is randomized using a seed value. The seed is
automatically generated on the first explicit or implicit call to
:func:`json_object()`, if :func:`json_object_seed()` has not been
called beforehand.

The seed is generated by using operating system's entropy sources if
they are available (``/dev/urandom``, ``CryptGenRandom()``). The
initialization is done in as thread safe manner as possible, by using
architecture specific lockless operations if provided by the platform
or the compiler.

If you're using threads, it's recommended to autoseed the hashtable
explicitly before spawning any threads by calling
``json_object_seed(0)`` , especially if you're unsure whether the
initialization is thread safe on your platform.


Memory allocation functions
===========================

Memory allocation functions should be set at most once, and only on
program startup. See :ref:`apiref-custom-memory-allocation`.


Locale
======

Jansson works fine under any locale.

However, if the host program is multithreaded and uses ``setlocale()``
to switch the locale in one thread while Jansson is currently encoding
or decoding JSON data in another thread, the result may be wrong or
the program may even crash.

Jansson uses locale specific functions for certain string conversions
in the encoder and decoder, and then converts the locale specific
values to/from the JSON representation. This fails if the locale
changes between the string conversion and the locale-to-JSON
conversion. This can only happen in multithreaded programs that use
``setlocale()``, because ``setlocale()`` switches the locale for all
running threads, not only the thread that calls ``setlocale()``.

If your program uses ``setlocale()`` as described above, consider
using the thread-safe ``uselocale()`` instead.
