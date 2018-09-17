#include "addon.h"

#define REPORT_EVERY 1000

// The secondary thread produces prime numbers using a very inefficient
// algorithm and calls into JavaScript with every REPORT_EVERYth prime number.
// After each call it checks whether any of the previous calls have produced a
// return value, and, if so, whether that return value is `false`. A `false`
// return value indicates that the JavaScript side is no longer interested in
// receiving any values and that the thread and the thread-safe function are to
// be cleaned up. On the JavaScript thread, this is marked in
// `addon_data->js_accepts`. When set to `false`, the JavaScript thread will not
// access the thread items any further, so they can be safely deleted on this
// thread.
void PrimeThread (void* data) {
  AddonData* addon_data = (AddonData*) data;
  int idx_outer, idx_inner;
  int prime_count = 0;
  ThreadItem* first = NULL;
  ThreadItem* current = NULL;
  ThreadItem* previous = NULL;
  ThreadItem* returned = NULL;

  // Check each integer whether it's a prime.
  for (idx_outer = 2 ;; idx_outer++) {

    // Check whether `idx_outer` is divisible by anything up to and not
    // including itself.
    for (idx_inner = 2;
        idx_inner < idx_outer && idx_outer % idx_inner != 0;
        idx_inner++);

    // If we find a prime, and it is REPORT_EVERY primes away from the previous
    // prime we found, then we send it to JavaScript.
    if (idx_inner >= idx_outer && (++prime_count % REPORT_EVERY) == 0) {
      // Create a new thread item and attach it to the list of outstanding
      // `ThreadItem` structures representing calls into JavaScript for which
      // no return value has yet been established.
      current = memset(malloc(sizeof(*current)), 0, sizeof(*current));
      current->the_prime = idx_outer;
      current->call_has_returned = false;
      current->return_value = false;
      current->next = first;
      first = current;

      // Pass the new item into JavaScript.
      assert(napi_call_threadsafe_function(addon_data->tsfn,
                                           first,
                                           napi_tsfn_blocking) == napi_ok);
    }

    // Pass over all outstanding thread items and check whether any of them have
    // returned.
    for (current = first, previous = NULL, returned = NULL;
        current != NULL && returned == NULL;
        previous = current,
        current = current->next) {
      uv_mutex_lock(&(addon_data->check_status_mutex));
      if (current->call_has_returned) {
        // Unhook the call that has returned from the list.
        if (previous != NULL) {
          previous->next = current->next;
        } else {
          first = current->next;
        }
        returned = current;
      }
      uv_mutex_unlock(&(addon_data->check_status_mutex));
    }

    // Process a return value. Free the `ThreadItem` that returned it, and break
    // out of the loop if the return value was `false`.
    if (returned != NULL) {
      // Save the return value to a local variable because we have to check it
      // after having freed the structure wherein it is stored.
      bool return_value = returned->return_value;
      free(returned);
      if (!return_value) {
        break;
      }
    }
  }

  // Before terminating the thread we free the remaining queue items. `CallJs`
  // will be called with pointers to these items, perhaps after this thread has
  // already freed them, but that's OK, because on the JavaScript thread
  // `addon_data->js_accepts` will certainly have been set to `false`, and so
  // `CallJs` will not dereference the stale pointers.
  for (current = first; current != NULL;) {
    previous = current;
    current = current->next;
    free(previous);
  }

  // Release the thread-safe function. This causes it to be cleaned up in the
  // background.
  assert(napi_release_threadsafe_function(addon_data->tsfn,
                                          napi_tsfn_release) == napi_ok);
}

// We use a separate binding to register a return value for a given call into
// JavaScript, represented by a `ThreadItem` object on both the JavaScript side
// and the native side. This allows the JavaScript side to asynchronously
// determine the return value.
napi_value RegisterReturnValue(napi_env env, napi_callback_info info) {
  // This function accepts two parameters:
  // 1. The thread item passed into JavaScript via `CallJs`, and
  // 2. The desired return value.
  size_t argc = 2;
  napi_value argv[2];
  napi_value constructor;
  AddonData* addon_data;
  bool right_instance, return_value;
  ThreadItem* item;

  // Retrieve the parameters with which this function was called.
  assert(napi_get_cb_info(env,
                          info,
                          &argc,
                          argv,
                          NULL,
                          (void*)&addon_data) == napi_ok);

  // If this function recorded a return value of `false` before, it means that
  // the thread and the associated thread-safe function are shutting down. This,
  // in turn means that the wrapped `ThreadItem` that was received in the first
  // argument may also be stale. Our best strategy is to do nothing and return
  // to JavaScript.
  if (!addon_data->js_accepts) {
    return NULL;
  }

  assert(argc == 2 && "Exactly two arguments were received");

  // Retrieve the constructor for `ThreadItem` instances.
  assert(napi_get_reference_value(env,
                                  addon_data->thread_item_constructor,
                                  &constructor) == napi_ok);

  // Make sure the first parameter is an instance of the `ThreadItem` class.
  // This type check ensures that there *is* a pointer stored inside the
  // JavaScript object, and that the pointer is to a `ThreadItem` structure.
  assert(napi_instanceof(env,
                         argv[0],
                         constructor,
                         &right_instance) == napi_ok);
  assert(right_instance && "First argument is a `ThreadItem`");

  // Retrieve the native data from the item.
  assert(napi_unwrap(env, argv[0], (void**)&item) == napi_ok);

  // Retrieve the desired return value.
  assert(napi_get_value_bool(env, argv[1], &return_value) == napi_ok);

  printf("RegisterReturnValue return_value %s\n", return_value ? "true" : "false");

  // Set `js_accepts` to false in case the JavaScript callback returned false.
  if (addon_data->js_accepts) {
    addon_data->js_accepts = return_value;
  }

  // Mark the thread item as resolved, and record the JavaScript return value.
  uv_mutex_lock(&(addon_data->check_status_mutex));
  item->call_has_returned = true;
  item->return_value = return_value;
  uv_mutex_unlock(&(addon_data->check_status_mutex));

  return NULL;
}

void consumeTokenJavascript (TokenType* tt) {
}

void produceTokenJavascript (TokenType* tt) {
}

napi_value Start2ThreadsTokenJavascript (napi_env env, AddonData* ad) {
  return NULL;
}
