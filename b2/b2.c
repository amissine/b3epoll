#include "b2.h"
/*
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
      current = (ThreadItem*)memset(malloc(sizeof(*current)), 0, sizeof(*current));
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

// This function is responsible for converting the native data coming in from
// the secondary thread to JavaScript values, and for calling the JavaScript
// function. It may also be called with `env` and `js_cb` set to `NULL` when
// Node.js is terminating and there are items coming in from the secondary
// thread left to process. In that case, this function does nothing, since it is
// the secondary thread that frees the items.
void CallJs(napi_env env, napi_value js_cb, void* context, void* data) {
  AddonData* addon_data = (AddonData*)context;
  napi_value constructor;

  // The semantics of this example are such that, once the JavaScript returns
  // `false`, the `ThreadItem` structures can no longer be accessed, because the
  // thread terminates and frees them all. Thus, we record the instant when
  // JavaScript returns `false` by setting `addon_data->js_accepts` to `false`
  // in `RegisterReturnValue` below, and we use the value here to decide whether
  // the data coming in from the secondary thread is stale or not.
  if (addon_data->js_accepts && !(env == NULL || js_cb == NULL)) {
    napi_value undefined, argv[1];
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

    // Call the JavaScript function with the item as wrapped into an instance of
    // the JavaScript `ThreadItem` class and the prime.
    assert(napi_call_function(env, undefined, js_cb, 1, argv, NULL) == napi_ok);
  }
}

// This function is responsible for converting the native data coming in from
// the non-main thread to JavaScript values, and for calling the JavaScript
// function. It may also be called with `env` and `js_cb` set to `NULL` when
// Node.js is terminating and there are items coming in from the secondary
// thread left to process. In that case, this function does nothing, since it is
// the secondary thread that frees the items.
void CallJs_onToken(napi_env env, napi_value js_cb, void* context, void* data) {
  AddonData* addon_data = (AddonData*)context;
  napi_value constructor;
  napi_value undefined, argv[1];

  // Retrieve the JavaScript `undefined` value. This will serve as the `this`
  // value for the function call.
  assert(napi_get_undefined(env, &undefined) == napi_ok);
    
  // Retrieve the constructor for the JavaScript class from which the item
  // holding the native data will be constructed.
  assert(napi_get_reference_value(env,
                                  addon_data->token_type_constructor,
                                  &constructor) == napi_ok);

  // Construct a new instance of the JavaScript class to hold the native item.
  assert(napi_new_instance(env, constructor, 0, NULL, &argv[0]) == napi_ok);

  // Associate the native token with the newly constructed JavaScript object.
  assert(napi_wrap(env, argv[0], data, NULL, NULL, NULL) == napi_ok);

  // Call the JavaScript function with the token wrapped into an instance of
  // the JavaScript `TokenType` class.
  assert(napi_call_function(env, undefined, js_cb, 1, argv, NULL) == napi_ok);
}

static bool is_instanceof (napi_env env, napi_ref cons_ref, napi_value value) {
  bool validate;
  napi_value constructor;
  assert(napi_ok == napi_get_reference_value(env, cons_ref, &constructor));
  assert(napi_ok == napi_instanceof(env, value, constructor, &validate));
  return validate;
}

// Notify the token producer thread.
napi_value NotifyTokenProducer (napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  AddonData* ad;
  ThreadItem* item;
  TokenType* token = memset(malloc(sizeof(*token)), 0, sizeof(*token));

  // Retrieve the argument.
  assert(napi_get_cb_info(env,
                          info,
                          &argc,
                          argv,
                          NULL,
                          (void*)&ad) == napi_ok);
  assert(argc == 1 && "Exactly one argument was received");

  // Make sure the argument is an instance of the `ThreadItem` class.
  // This type check ensures that there *is* a pointer stored inside the
  // JavaScript object, and that the pointer is to a `ThreadItem` structure.
  assert(is_instanceof(env, ad->thread_item_constructor, argv[0]));

  // Retrieve the native data from the item.
  assert(napi_unwrap(env, argv[0], (void**)&item) == napi_ok);

  // Initialise the token with the item data, queue it and notify 
  // the producer thread.
  initTokenType(token, item->the_prime);
  uv_mutex_lock(&ad->tokenProducingMutex);
  fifoIn(&ad->queue, &token->t);
  uv_cond_signal(&ad->tokenProducing);
  uv_mutex_unlock(&ad->tokenProducingMutex);
  return NULL;
}

napi_value tokenConsumed (napi_env env, AddonData* ad, napi_value* argv) {
  TokenType* t;
  bool b;

  // Retrieve the native token.
  assert(napi_ok == napi_unwrap(env, *argv++, (void**)&t));

  // Retrieve the boolean argument (presently unused).
  assert(napi_ok == napi_get_value_bool(env, *argv, &b));

  // Notify the consumer thread that the token has been consumed.
  uv_mutex_lock(&ad->tokenConsumingMutex);
  t->theDelay = 0ll;
  uv_cond_signal(&ad->tokenConsuming);
  uv_mutex_unlock(&ad->tokenConsumingMutex);
  return NULL;
}

// We use a separate binding to register a return value for a given call into
// JavaScript, represented by a `ThreadItem` or a `TokenType`object on both 
// the JavaScript side and the native side. This allows the JavaScript side to 
// asynchronously determine the return value.
napi_value RegisterReturnValue(napi_env env, napi_callback_info info) {
  // This function accepts two parameters:
  // 1. The JavaScript object passed into JavaScript via either `CallJs`
  //    or `CallJs_onToken`, and
  // 2. The desired return value.
  size_t argc = 2;
  napi_value argv[2];
  AddonData* addon_data;
  bool return_value;
  ThreadItem* item;

  // Retrieve the parameters with which this function was called.
  assert(napi_get_cb_info(env, info, &argc, argv, 
        NULL, (void*)&addon_data) == napi_ok);
  assert(argc == 2 && "Exactly two arguments were received");

  // Check if a `TokenType` object has been passed
  if (is_instanceof(env, addon_data->token_type_constructor, argv[0]))
    return tokenConsumed(env, addon_data, argv);

  // If this function recorded a return value of `false` before, it means that
  // the thread and the associated thread-safe function are shutting down. This,
  // in turn means that the wrapped `ThreadItem` that was received in the first
  // argument may also be stale. Our best strategy is to do nothing and return
  // to JavaScript.
  if (!addon_data->js_accepts) {
    return NULL;
  }

  // Make sure the first parameter is an instance of the `ThreadItem` class.
  // This type check ensures that there *is* a pointer stored inside the
  // JavaScript object, and that the pointer is to a `ThreadItem` structure.
  assert(is_instanceof(env, addon_data->thread_item_constructor, argv[0]));

  // Retrieve the native data from the item.
  assert(napi_unwrap(env, argv[0], (void**)&item) == napi_ok);

  // Retrieve the desired return value.
  assert(napi_get_value_bool(env, argv[1], &return_value) == napi_ok);

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

// Constructor for instances of the `ThreadItem` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `ThreadItem`
// structure.
napi_value ThreadItemConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

// Constructor for instances of the `TokenType` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `TokenType`
// structure.
napi_value TokenTypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

// Getter for the `prime` property of the `TokenType` class.
napi_value GetTokenPrime(napi_env env, napi_callback_info info) {
  napi_value jsthis, prime_property;
  AddonData* ad;
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&ad));
  assert(is_instanceof(env, ad->token_type_constructor, jsthis));
  TokenType* token;
  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&token));
  assert(napi_ok == napi_create_int32(env, 
        token->thePrime, 
        &prime_property));
  return prime_property;
}

// Getter for the `delay` property of the `TokenType` class.
napi_value GetTokenDelay(napi_env env, napi_callback_info info) {
  napi_value jsthis, property;
  AddonData* ad;
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&ad));
  assert(is_instanceof(env, ad->token_type_constructor, jsthis));
  TokenType* token;
  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&token));
  assert(napi_ok == napi_create_int64(env, 
        token->theDelay, 
        &property));
  return property;
}

// Getter for the `prime` property of the `ThreadItem` class.
napi_value GetPrime(napi_env env, napi_callback_info info) {
  napi_value jsthis, prime_property;
  AddonData* ad;
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&ad));
  assert(is_instanceof(env, ad->thread_item_constructor, jsthis));
  ThreadItem* item;
  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&item));
  assert(napi_ok == napi_create_int32(env, 
        item->the_prime, 
        &prime_property));
  return prime_property;
}

void consumeTokenJavascript (TokenType* tt, AddonData* ad) {

  // Set the consumer - producer delay in the tt->theDelay field.
  long long int p = tt->theDelay;
  struct timeval timer_us;
  if (gettimeofday(&timer_us, NULL) == 0) {
    tt->theDelay = ((long long int) timer_us.tv_sec) * 1000000ll +
      (long long int) timer_us.tv_usec - p;
  }
  else tt->theDelay = -1ll;

  // Pass the consumed token to the 'onToken' JavaScript function,
  // then wait until the main thread is done with the token.
  assert(napi_ok == napi_call_threadsafe_function(ad->onToken,
        tt, napi_tsfn_blocking));
  if (tt->theDelay != 0ll) {
    printf("consumeTokenJavascript: wait for the token to be consumed\n");
    uv_mutex_lock(&ad->tokenConsumingMutex);

    // Wait for the token to be consumed
    while (ad->js_accepts && tt->theDelay != 0ll)
      uv_cond_wait(&ad->tokenConsuming, &ad->tokenConsumingMutex);
    uv_mutex_unlock(&ad->tokenConsumingMutex);
  }
}

void produceTokenJavascript (TokenType* tt, AddonData* ad) {
  struct fifo* t;
  if (ad->js_accepts && ad->queue.size == 0)
    printf("produceTokenJavascript: wait for a token from the main thread\n");

  uv_mutex_lock(&ad->tokenProducingMutex);
  if (ad->queue.size > 0) { // token(s) have been produced by the main thread
    t = fifoOut(&ad->queue); // remove the first token from the queue
    if (t) { // if it's not NULL, copy it to the shared buffer and return
      memcpy(tt, t, sizeof(TokenType));
      free(t);
      uv_mutex_unlock(&ad->tokenProducingMutex); 
      return;
    }
  }

  // Wait for a token from the main thread
  while (ad->js_accepts && ad->queue.size == 0)
    uv_cond_wait(&ad->tokenProducing, &ad->tokenProducingMutex);

  if (ad->js_accepts) {
    t = fifoOut(&ad->queue); // remove it from the queue,
    memcpy(tt, t, sizeof(TokenType)); // copy to the shared buffer and return
    free(t);
  }
  uv_mutex_unlock(&ad->tokenProducingMutex);
}

napi_value Start2ThreadsTokenJavascript (AddonData* ad) {

  // Reset the shared buffer.
  produceCount = 0;
  consumeCount = 0;

  // Create and start the consumer thread.
  assert(uv_thread_create(&ad->consumerThread, consumeTokens, ad) == 0);

  // Create and start the producer thread.
  assert(uv_thread_create(&ad->producerThread, produceTokens, ad) == 0);

  return NULL;
}
*/
