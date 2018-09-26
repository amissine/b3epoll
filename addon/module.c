#include "addon.h"

static void addon_is_unloading(napi_env env, void* data, void* hint) {

  printf("addon_is_unloading started\n");

  AddonData* addon_data = (AddonData*)data;
  uv_mutex_destroy(&addon_data->check_status_mutex);
  uv_mutex_destroy(&addon_data->tokenProducedMutex);
  uv_mutex_destroy(&addon_data->tokenProducingMutex);
  uv_mutex_destroy(&addon_data->tokenConsumedMutex);
  uv_mutex_destroy(&addon_data->tokenConsumingMutex);
  uv_cond_destroy(&addon_data->tokenProduced);
  uv_cond_destroy(&addon_data->tokenProducing);
  uv_cond_destroy(&addon_data->tokenConsumed);
  uv_cond_destroy(&addon_data->tokenConsuming);
  assert(napi_delete_reference(env,
                               addon_data->thread_item_constructor) == napi_ok);
  free(data);
}

// When the thread is finished we join it to prevent memory leaks. We can safely
// set `addon_data->tsfn` to NULL, because the thread-safe function will be
// cleaned up in the background in response to the secondary thread having
// called `napi_release_threadsafe_function()`.
static void ThreadFinished(napi_env env, void* data, void* context) {

  printf("ThreadFinished started\n"); // TODO: the onToken finalization

  (void) context;
  AddonData* addon_data = (AddonData*)data;
  assert(uv_thread_join(&(addon_data->the_thread)) == 0);
  addon_data->tsfn = NULL;
}

// This binding can be called from JavaScript to start the producer-consumer
// pair of threads.
static napi_value Start(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value js_cb[2], name1, name2;
  AddonData* ad;
  char desc2[] = "b3epoll TOKEN_JAVASCRIPT producer";
  char desc1[] = "b3epoll TOKEN_JAVASCRIPT consumer";

  // The binding accepts two parameters - the JavaScript callback functions
  // `onToken` and `onItem`.
  assert(napi_get_cb_info(
        env, info, &argc, js_cb, NULL, (void*)&ad) == napi_ok);

  // We do not create a second thread if one is already running.
  assert(ad->tsfn == NULL && "Work already in progress");

  ad->js_accepts = true;

  // This string describes the asynchronous work.
  assert(napi_create_string_utf8(
        env, desc2, NAPI_AUTO_LENGTH, &name2) == napi_ok);

  assert(napi_ok = napi_create_string_utf8(env, desc1, NAPI_AUTO_LENGTH, &name1));

  // The thread-safe function will be created with an unlimited queue and with
  // an initial thread count of 1. The secondary thread will release the
  // thread-safe function, decreasing its thread count to 0, thereby setting off
  // the process of cleaning up the thread-safe function.
  assert(napi_ok = napi_create_threadsafe_function(env, js_cb[2], NULL, name2,
        0, 1, ad, ThreadFinished, ad, CallJs, &ad->tsfn));

  assert(napi_ok = napi_create_threadsafe_function(env, js_cb[1], NULL, name1,
        0, 1, ad, ThreadFinished, ad, CallJs_onToken, &ad->onToken));

  // Create the thread that will produce primes and that will call into
  // JavaScript using the thread-safe function.
  assert(uv_thread_create(&(ad->the_thread), PrimeThread, ad) == 0);

  return Start2Threads(ad);
}

static inline int initAddonData (AddonData* ad) {
  fifoInit(&ad->queue);
  return (uv_mutex_init(&ad->check_status_mutex) == 0) &&
    (uv_mutex_init(&ad->tokenProducedMutex) == 0) &&
    (uv_mutex_init(&ad->tokenConsumedMutex) == 0) &&
    (uv_mutex_init(&ad->tokenProducingMutex) == 0) &&
    (uv_mutex_init(&ad->tokenConsumingMutex) == 0) &&
    (uv_cond_init(&ad->tokenProduced) == 0) &&
    (uv_cond_init(&ad->tokenConsumed) == 0) &&
    (uv_cond_init(&ad->tokenProducing) == 0) &&
    (uv_cond_init(&ad->tokenConsuming) == 0);
}

static inline void defineThreadItemClass (napi_env env, AddonData* ad) {
  napi_value thread_item_class;
  napi_property_descriptor properties[] = {
    { "prime", 0, 0, GetPrime, 0, 0, napi_enumerable, ad }
  };
  size_t count = sizeof(properties) / sizeof(properties[0]);
  assert(napi_define_class(env,
                           "ThreadItem",
                           NAPI_AUTO_LENGTH,
                           ThreadItemConstructor,
                           ad,
                           count,
                           properties,
                           &thread_item_class) == napi_ok);
  assert(napi_create_reference(env,
                               thread_item_class,
                               1,
                               &ad->thread_item_constructor) == napi_ok);
}

static inline napi_value bindings (
    napi_env env, napi_value exports, AddonData* ad) {
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

// Initialize an instance of this addon. This function may be called multiple
// times if multiple instances of Node.js are running on multiple threads, or if
// there are multiple Node.js contexts running on the same thread. The return
// value and the formal parameters in comments remind us that the function body
// that follows, within which we initialize the addon, has available to it the
// variables named in the formal parameters, and that it must return a
// `napi_value`.
/*napi_value*/ NAPI_MODULE_INIT(/*napi_env env, napi_value exports*/) {
  // Create the native data that will be associated with this instance of the
  // addon.
  AddonData* addon_data =
      memset(malloc(sizeof(*addon_data)), 0, sizeof(*addon_data));

  // Attach the addon data to the exports object to ensure that they are
  // destroyed together.
  assert(napi_wrap(env,
                   exports,
                   addon_data,
                   addon_is_unloading,
                   NULL,
                   NULL) == napi_ok);

  // Initialize the various members of the `AddonData` associated with this
  // addon instance, define ThreadItemClass.
  assert(initAddonData(addon_data));
  defineThreadItemClass(env, addon_data);

  // Expose and return the bindings this addon provides.
  return bindings(env, exports, addon_data);
}
