#include "addon.h"

// Constructor for instances of the `ThreadItem` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `ThreadItem`
// structure.
static napi_value ThreadItemConstructor(napi_env env, napi_callback_info info) {
  return NULL;
}

static void addon_is_unloading(napi_env env, void* data, void* hint) {
  AddonData* addon_data = (AddonData*)data;
  uv_mutex_destroy(&(addon_data->check_status_mutex));
  assert(napi_delete_reference(env,
                               addon_data->thread_item_constructor) == napi_ok);
  free(data);
}

// This function is responsible for converting the native data coming in from
// the secondary thread to JavaScript values, and for calling the JavaScript
// function. It may also be called with `env` and `js_cb` set to `NULL` when
// Node.js is terminating and there are items coming in from the secondary
// thread left to process. In that case, this function does nothing, since it is
// the secondary thread that frees the items.
static void CallJs(napi_env env, napi_value js_cb, void* context, void* data) {
  AddonData* addon_data = (AddonData*)context;
  napi_value constructor;

  // The semantics of this example are such that, once the JavaScript returns
  // `false`, the `ThreadItem` structures can no longer be accessed, because the
  // thread terminates and frees them all. Thus, we record the instant when
  // JavaScript returns `false` by setting `addon_data->js_accepts` to `false`
  // in `RegisterReturnValue` below, and we use the value here to decide whether
  // the data coming in from the secondary thread is stale or not.
  if (addon_data->js_accepts && !(env == NULL || js_cb == NULL)) {
    napi_value undefined, argv[2];
    // Retrieve the JavaScript `undefined` value. This will serve as the `this`
    // value for the function call.
    assert(napi_get_undefined(env, &undefined) == napi_ok);

    // Retrieve the constructor for the JavaScript class from which the item
    // holding the native data will be constructed.
    assert(napi_get_reference_value(env,
                                    addon_data->thread_item_constructor,
                                    &constructor) == napi_ok);

    // Construct a new instance of the JavaScript class to hold the native item.
    assert(napi_new_instance(env, constructor, 0, NULL, &argv[0]) == napi_ok);

    // Associate the native item with the newly constructed JavaScript object.
    // We assume that the JavaScript side will eventually pass this JavaScript
    // object back to us via `RegisterReturnValue`, which will allow the
    // eventual deallocation of the native data. That's why we do not provide a
    // finalizer here.
    assert(napi_wrap(env, argv[0], data, NULL, NULL, NULL) == napi_ok);

    // Convert the prime number to a number `napi_value` we can pass into
    // JavaScript.
    assert(napi_create_int32(env,
                             ((ThreadItem*)data)->the_prime,
                             &argv[1]) == napi_ok);

    // Call the JavaScript function with the item as wrapped into an instance of
    // the JavaScript `ThreadItem` class and the prime.
    assert(napi_call_function(env, undefined, js_cb, 2, argv, NULL) == napi_ok);
  }
}

// When the thread is finished we join it to prevent memory leaks. We can safely
// set `addon_data->tsfn` to NULL, because the thread-safe function will be
// cleaned up in the background in response to the secondary thread having
// called `napi_release_threadsafe_function()`.
static void ThreadFinished(napi_env env, void* data, void* context) {
  (void) context;
  AddonData* addon_data = (AddonData*)data;
  assert(uv_thread_join(&(addon_data->the_thread)) == 0);
  addon_data->tsfn = NULL;
}

// This binding can be called from JavaScript to start the producer-consumer
// pair of threads.
static napi_value Start(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value js_cb, work_name;
  AddonData* addon_data;

  // The binding accepts one parameter - the JavaScript callback function to
  // call.
  assert(napi_get_cb_info(env,
                          info,
                          &argc,
                          &js_cb,
                          NULL,
                          (void*)&addon_data) == napi_ok);

  // We do not create a second thread if one is already running.
  assert(addon_data->tsfn == NULL && "Work already in progress");

  addon_data->js_accepts = true;

  // This string describes the asynchronous work.
  assert(napi_create_string_utf8(env,
                                 "Thread-safe Function Round Trip Example",
                                 NAPI_AUTO_LENGTH,
                                 &work_name) == napi_ok);

  // The thread-safe function will be created with an unlimited queue and with
  // an initial thread count of 1. The secondary thread will release the
  // thread-safe function, decreasing its thread count to 0, thereby setting off
  // the process of cleaning up the thread-safe function.
  assert(napi_create_threadsafe_function(env,
                                         js_cb,
                                         NULL,
                                         work_name,
                                         0,
                                         1,
                                         addon_data,
                                         ThreadFinished,
                                         addon_data,
                                         CallJs,
                                         &addon_data->tsfn) == napi_ok);

  // Create the thread that will produce primes and that will call into
  // JavaScript using the thread-safe function.
  assert(uv_thread_create(&(addon_data->the_thread), PrimeThread, addon_data) == 0);

  return NULL;
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
  // addon instance.
  assert(uv_mutex_init(&(addon_data->check_status_mutex)) == 0);

  napi_value thread_item_class;

  assert(napi_define_class(env,
                           "ThreadItem",
                           NAPI_AUTO_LENGTH,
                           ThreadItemConstructor,
                           addon_data,
                           0,
                           NULL,
                           &thread_item_class) == napi_ok);
  assert(napi_create_reference(env,
                               thread_item_class,
                               1,
                               &(addon_data->thread_item_constructor)) ==
                                  napi_ok);

  // Expose the bindings this addon provides.
  napi_property_descriptor export_properties[] = {
    {
      "start",
      NULL,
      Start,
      NULL,
      NULL,
      NULL,
      napi_default,
      addon_data
    },
    {
      "stop",
      NULL,
      RegisterReturnValue,
      NULL,
      NULL,
      NULL,
      napi_default,
      addon_data
    }
  };
  assert(napi_define_properties(env,
                                exports,
                                sizeof(export_properties) /
                                    sizeof(export_properties[0]),
                                export_properties) == napi_ok);

  return exports;
}
