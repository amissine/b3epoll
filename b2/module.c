#include "b2.h"

static void freeModuleData (napi_env env, void* data, void* hint) {

  printf("freeModuleData started\n");

  ModuleData* md = (ModuleData*) data;
  assert(napi_ok == napi_delete_reference(env, md->tt_constructor));
  free(data);
}
/*

// When the thread is finished we join it to prevent memory leaks. We can safely
// set `addon_data->tsfn` to NULL, because the thread-safe function will be
// cleaned up in the background in response to the secondary thread having
// called `napi_release_threadsafe_function()`.
static void ThreadFinished (napi_env env, void* data, void* context) {
  AddonData* ad = (AddonData*)data;

  printf("ThreadFinished started, ad->js_accepts: %s\n", 
      ad->js_accepts ? "true" : "false");

  if (!ad->tsfn) return; // ad->onToken is being finalized

  assert(uv_thread_join(&ad->the_thread) == 0);
  ad->tsfn = NULL;

  // Release the thread-safe function. This causes it to be cleaned up in the
  // background.
  assert(napi_release_threadsafe_function(ad->onToken,
                                          napi_tsfn_release) == napi_ok);

  uv_mutex_lock(&ad->tokenProducedMutex);
  uv_cond_signal(&ad->tokenProduced);
  uv_mutex_unlock(&ad->tokenProducedMutex);

  uv_mutex_lock(&ad->tokenProducingMutex);
  uv_cond_signal(&ad->tokenProducing);
  uv_mutex_unlock(&ad->tokenProducingMutex);

  uv_mutex_lock(&ad->tokenConsumedMutex);
  uv_cond_signal(&ad->tokenConsumed);
  uv_mutex_unlock(&ad->tokenConsumedMutex);

  uv_mutex_lock(&ad->tokenConsumingMutex);
  uv_cond_signal(&ad->tokenConsuming);
  uv_mutex_unlock(&ad->tokenConsumingMutex);

  assert(uv_thread_join(&ad->producerThread) == 0);
  assert(uv_thread_join(&ad->consumerThread) == 0);
  
  // Empty the queue of tokens that have not been produced.
  struct fifo* t;
  while ((t = fifoOut(&ad->queue)) != NULL) {
    printf("ThreadFinished token->thePrime: %d\n", ((TokenType*)t)->thePrime);
    free(t);
  }
  printf("ThreadFinished returning, ad->queue.size: %zu\n", ad->queue.size);
}

// This binding can be called from JavaScript to start the producer-consumer
// pair of threads and the original thread that generates prime numbers.
static napi_value Start (napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value js_cb[2], name1, name2;
  AddonData* ad;
  char desc2[] = "b3epoll TOKEN_JAVASCRIPT token generator";
  char desc1[] = "b3epoll TOKEN_JAVASCRIPT token consumer";

  // The binding accepts two parameters - the JavaScript callback functions
  // `onToken` and `onItem`.
  assert(napi_get_cb_info(
        env, info, &argc, js_cb, NULL, (void*)&ad) == napi_ok);

  // We do not create a second thread if one is already running.
  assert(ad->tsfn == NULL && "Work already in progress");

  ad->js_accepts = true;

  // These strings describe the asynchronous work.
  assert(napi_create_string_utf8(env, desc2, NAPI_AUTO_LENGTH, &name2) == napi_ok);
  assert(napi_ok == napi_create_string_utf8(env, desc1, NAPI_AUTO_LENGTH, &name1));

  // The thread-safe function will be created with an unlimited queue and with
  // an initial thread count of 1. The secondary thread will release the
  // thread-safe function, decreasing its thread count to 0, thereby setting off
  // the process of cleaning up the thread-safe function.
  assert(napi_ok == napi_create_threadsafe_function(env, js_cb[1], NULL, name2,
        0, 1, ad, ThreadFinished, ad, CallJs, &ad->tsfn));
  assert(napi_ok == napi_create_threadsafe_function(env, js_cb[0], NULL, name1,
        0, 1, ad, ThreadFinished, ad, CallJs_onToken, &ad->onToken));

  // Create the thread that will produce primes and that will call into
  // JavaScript using the thread-safe function.
  assert(uv_thread_create(&(ad->the_thread), PrimeThread, ad) == 0);

  // Create the producer-consumer pair of threads.
  return Start2Threads(ad);
}
*/

static inline int initModuleData (ModuleData* md) {
  fifoInit(&md->b2instances);
 
  // Define the token type. The md->tt_constructor napi_ref will be deleted
  // during the 'freeModuleData' call.
  char* propNamesTT[3] = { "sid", "message", "delay" };
  napi_property_descriptor pTT[3];
  napi_callback settersTT[3] = { NULL, NULL, NULL },
               gettersTT[3] = { GetTokenSid, GetTokenMessage, GetTokenDelay }; 
  defObj_n_props(env, md, "TokenType", TokenTypeConstructor,
      &md->tt_constructor, 3, pTT, propNamesTT, gettersTT, settersTT);

  return TRUE;
}

static inline napi_value bindings (
    napi_env env, napi_value exports, ModuleData* md) {
  napi_property_descriptor export_properties[] = {
    { "start", 0, Start, 0, 0, 0, napi_default, ad },
    { "doneWith", 0, RegisterReturnValue, 0, 0, 0, napi_default, ad },
    { "produceToken", 0, NotifyTokenProducer, 0, 0, 0, napi_default, ad }
  };
  size_t count = sizeof(export_properties) / sizeof(export_properties[0]);
  assert(napi_define_properties(env,
                                exports,
                                count,
                                export_properties) == napi_ok);
  return exports;
}

// Initialize an instance of this module. This function may be called multiple
// times if multiple instances of Node.js are running on multiple threads, or if
// there are multiple Node.js contexts running on the same thread. The return
// value and the formal parameters in comments remind us that the function body
// that follows, within which we initialize the addon, has available to it the
// variables named in the formal parameters, and that it must return a
// `napi_value`.
/*napi_value*/ NAPI_MODULE_INIT(/*napi_env env, napi_value exports*/) {
  // Create the native data that will be associated with this instance of the
  // module.
  ModuleData* md = memset(malloc(sizeof(*md)), 0, sizeof(*md));

  // Attach the module data to the exports object to ensure that they are
  // destroyed together. Initialize the module data.
  assert(napi_ok == napi_wrap(env, exports, md, freeModuleData, 0, 0));
  assert(initModuleData(md));
  
  // Expose and return the bindings this addon provides.
  return bindings(env, exports, md);
}
