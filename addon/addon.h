#ifndef ADDON_H
#define ADDON_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <node_api.h>

// An item that will be generated from the thread, passed into JavaScript, and
// ultimately marked as resolved when the JavaScript passes it back into the
// addon instance with a return value.
typedef struct ThreadItem {
  // This field is read-only once set, so it need not be protected by the mutex.
  int the_prime;

  // This field is only accessed from the secondary thread, so it also need not
  // be protected by the mutex.
  struct ThreadItem* next;

  // These two values must be protected by the mutex.
  bool call_has_returned;
  bool return_value;
} ThreadItem;

// The data associated with an instance of the addon. This takes the place of
// global static variables, while allowing multiple instances of the addon to
// co-exist.
typedef struct {
  uv_mutex_t check_status_mutex, tokenProducedMutex, tokenConsumedMutex;
  uv_cond_t tokenProduced, tokenConsumed;
  uv_thread_t the_thread, producerThread, consumerThread;
  napi_threadsafe_function tsfn;
  napi_ref thread_item_constructor;
  bool js_accepts;
} AddonData;

#ifdef TOKEN_JAVASCRIPT
// These definitions are used in the beginning of the development. The data
// are prime numbers accompanied with the time it took to get the. The both
// sides of the shared buffer are the JavaScript functions.

// The data in the shared buffer.
typedef struct {
  int thePrime;
  int theDelay;
} TokenType;

#define produceToken produceTokenJavascript
#define consumeToken consumeTokenJavascript

void consumeTokenJavascript (TokenType*);
void produceTokenJavascript (TokenType*);
void PrimeThread (void* data); 
napi_value RegisterReturnValue (napi_env env, napi_callback_info info);

#endif // TOKEN_JAVASCRIPT
#endif // ADDON_H
