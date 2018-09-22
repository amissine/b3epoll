#ifndef ADDON_H
#define ADDON_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <node_api.h>

#ifdef TOKEN_JAVASCRIPT
struct ThreadItem;
#endif // TOKEN_JAVASCRIPT

// The data associated with an instance of the addon. This takes the place of
// global static variables, while allowing multiple instances of the addon to
// co-exist.
typedef struct {
  uv_mutex_t check_status_mutex, tokenProducedMutex, tokenConsumedMutex;
#ifdef TOKEN_JAVASCRIPT
  uv_mutex_t tokenProducingMutex, tokenConsumingMutex;
  uv_cond_t tokenProducing, tokenConsuming;
  struct ThreadItem* lastProduced;
#endif // TOKEN_JAVASCRIPT  
  uv_cond_t tokenProduced, tokenConsumed;
  uv_thread_t the_thread, producerThread, consumerThread;
  napi_threadsafe_function tsfn;
  napi_ref thread_item_constructor;
  bool js_accepts;
} AddonData;

void produceTokens (AddonData*);
void consumeTokens (AddonData*);

#ifdef TOKEN_JAVASCRIPT
// These definitions are used in the beginning of the development. The data
// are prime numbers accompanied with the time it took to get one. The both
// sides of the shared buffer are the JavaScript functions.

// The data in the shared buffer.
typedef struct {
  int thePrime;
  int theDelay;
} TokenType;

#define produceToken produceTokenJavascript
#define consumeToken consumeTokenJavascript
#define Start2Threads Start2ThreadsTokenJavascript

void consumeTokenJavascript (TokenType*, AddonData*);
void produceTokenJavascript (TokenType*, AddonData*);

// An item that will be generated from the thread, passed into JavaScript, and
// ultimately marked as resolved when the JavaScript passes it back into the
// addon instance with a return value.
typedef struct ThreadItem {
  // This field is read-only once set, so it need not be protected by the mutex.
  int the_prime;

  // This field is only accessed from the secondary thread, so it also need not
  // be protected by the mutex.
  struct ThreadItem* next;

  // This field is accessed from both the main loop thread (when another item
  // is produced) and the producer thread (when an item is used to construct
  // a token), so it must be protected by tokenProducingMutex.
  struct ThreadItem* prevProduced;

  // These two values must be protected by the mutex.
  bool call_has_returned;
  bool return_value;
} ThreadItem;

void CallJs(napi_env env, napi_value js_cb, void* context, void* data);
napi_value ThreadItemConstructor (napi_env env, napi_callback_info info);
napi_value GetPrime (napi_env env, napi_callback_info info);
void PrimeThread (void* data); 
napi_value RegisterReturnValue (napi_env env, napi_callback_info info);
napi_value NotifyTokenProducer (napi_env env, napi_callback_info info);
napi_value Start2ThreadsTokenJavascript (AddonData* ad);

#endif // TOKEN_JAVASCRIPT
#endif // ADDON_H
