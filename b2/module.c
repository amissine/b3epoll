#include "b2.h"

static void freeModuleData (napi_env env, void* data, void* hint) {

  printf("freeModuleData started\n");

  ModuleData* md = (ModuleData*) data;
  assert(napi_ok == napi_delete_reference(env, md->b2t_constructor));
  assert(napi_ok == napi_delete_reference(env, md->pt_constructor));
  assert(napi_ok == napi_delete_reference(env, md->ct_constructor));
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

// Constructor for instances of the `B2Type` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `B2Type`
// structure.
napi_value B2TypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value B2T_Open (napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value B2T_Close (napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value B2T_Producer (napi_env env, napi_callback_info info) {

  printf("B2T_Producer started\n");

  return NULL;
}

static napi_value B2T_Consumer (napi_env env, napi_callback_info info) {
  napi_value thisB2;
  ModuleData* md;
  struct B2 * b2;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &thisB2, (void*)&md));
  assert(napi_ok == napi_unwrap(env, thisB2, (void*)&b2));
  if (!b2->consumer.this)
    b2->consumer.this = newInstance(env, md->ct_constructor, &b2->consumer, 0, 0);

  printf("B2T_Consumer b2->b2t_this.sid: %u\n",
      b2->b2t_this.sid);

  return b2->consumer.this;
}

// Constructor for instances of the `ProducerType` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `ProducerType`
// structure.
napi_value ProducerTypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value PT_Send (napi_env env, napi_callback_info info) {
  return NULL;
}

// Constructor for instances of the `ConsumerType` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `ConsumerType`
// structure.
napi_value ConsumerTypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value CT_On (napi_env env, napi_callback_info info) {

  printf("CT_On started\n");

  return NULL;
}

static napi_value CT_DoneWith (napi_env env, napi_callback_info info) {
  return NULL;
}

// Constructor for instances of the `TokenType` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `TokenType`
// structure.
napi_value TokenTypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

// Getter for the `sid` property of the `TokenType` object.
static napi_value GetTokenSid (napi_env env, napi_callback_info info) {
  napi_value jsthis, property;
  ModuleData* md;
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&md));
  assert(is_instanceof(env, md->tt_constructor, jsthis));
  struct fifo *tt_this;
  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&tt_this));
  assert(napi_ok == napi_create_uint32(env, tt_this->sid, &property));
  return property;
}

// Getter for the `message` property of the `TokenType` object.
static napi_value GetTokenMessage (napi_env env, napi_callback_info info) {
  napi_value jsthis, property;
  ModuleData* md;
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&md));
  assert(is_instanceof(env, md->tt_constructor, jsthis));
  TokenType* token;
  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&token));
  assert(napi_ok == napi_create_string_utf8(
        env, token->theMessage, NAPI_AUTO_LENGTH, &property));
  return property;
}

// Getter for the `delay` property of the `TokenType` object.
static napi_value GetTokenDelay (napi_env env, napi_callback_info info) {
  napi_value jsthis, property;
  ModuleData* md;
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&md));
  assert(is_instanceof(env, md->tt_constructor, jsthis));
  TokenType* token;
  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&token));
  assert(napi_ok == napi_create_int64(env, token->theDelay, &property));
  return property;
}

static inline void initModuleData (napi_env env, ModuleData* md) {
  fifoInit(&md->b2instances);
 
  // Define the token type. The md->tt_constructor napi_ref will be deleted
  // during the 'freeModuleData' call.
  char* propNamesTT[3] = { "sid", "message", "delay" };
  napi_property_descriptor pTT[3];
  napi_callback methodsTT[3] = { NULL, NULL, NULL },
               gettersTT[3] = { GetTokenSid, GetTokenMessage, GetTokenDelay }; 
  defObj_n_props(env, md, "TokenType", TokenTypeConstructor,
      &md->tt_constructor, 3, pTT, propNamesTT, gettersTT, methodsTT);

  // Define the bounded buffer type. The md->b2t_constructor napi_ref 
  // will be deleted during the 'freeModuleData' call.
  char* propNamesB2T[4] = { "producer", "consumer", "open", "close" };
  napi_property_descriptor pB2T[4];
  napi_callback methodsB2T[4] = { NULL, NULL, B2T_Open, B2T_Close },
                gettersB2T[4] = { B2T_Producer, B2T_Consumer, NULL, NULL };
  defObj_n_props(env, md, "B2Type", B2TypeConstructor,
      &md->b2t_constructor, 4, pB2T, propNamesB2T, gettersB2T, methodsB2T);

  // Define the producer type. The md->pt_constructor napi_ref will be deleted
  // during the 'freeModuleData' call.
  char* propNamesPT[1] = { "send" };
  napi_property_descriptor pPT[1];
  napi_callback methodsPT[1] = { PT_Send },
                gettersPT[1] = { NULL };
  defObj_n_props(env, md, "ProducerType", ProducerTypeConstructor,
      &md->pt_constructor, 1, pPT, propNamesPT, gettersPT, methodsPT);

  // Define the consumer type. The md->ct_constructor napi_ref will be deleted
  // during the 'freeModuleData' call.
  char* propNamesCT[2] = { "on", "doneWith" };
  napi_property_descriptor pCT[2];
  napi_callback methodsCT[2] = { CT_On, CT_DoneWith },
                gettersCT[2] = { NULL, NULL };
  defObj_n_props(env, md, "ConsumerType", ConsumerTypeConstructor,
      &md->ct_constructor, 2, pCT, propNamesCT, gettersCT, methodsCT);
}

static void freeB2native (napi_env env, void* data, void* hint) {
  ModuleData* md = (ModuleData*)hint;
  struct B2 * b2 = (struct B2 *)data;
  struct fifo * q = &b2->b2t_this, * queue = &md->b2instances;
  struct fifo * p = q->out, * r = q->in;
  p->in = r; r->out = p; queue->size--;

  printf("freeB2native queue->size: %zu\n", queue->size);

  free(data);
}

// When the JavaScript side calls newB2 (for example, as follows:
//
//   var b2 = B2.newB2(producerConfigData, consumerConfigData)
//   b2.consumer.on('token', t => {
//     console.log(t)
//     b2.consumer.doneWith(t)
//   })
//   b2.consumer.on('close', () => {
//     console.log('The b2 threads are stopped now.')
//   })
//   b2.open()
//   b2.producer.send('Hello World')
//   setTimeout(() => b2.close(), 500)
//
// ), this function constructs the shared buffer and binds the producer/consumer
// pair to it. It returns back the JavaScript object that can be used to 
// start and stop the producer/consumer pair of threads, and to send and receive
// messages between the producer and the consumer.
napi_value newB2 (napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2], b2;
  ModuleData* md;
  struct B2 *structB2;

  assert(napi_ok == napi_get_cb_info(env, info, &argc, argv, 0, (void*)&md));
  structB2 = newB2native(env, argc, argv, md);
  b2 = newInstance(env, md->b2t_constructor, structB2, freeB2native, md);
  return b2;
}

static inline napi_value bindings (
    napi_env env, napi_value exports, ModuleData* md) {
  napi_property_descriptor p[] = {
    { "newB2", 0, newB2, 0, 0, 0, napi_default, md }
  };
  assert(napi_ok == napi_define_properties(env, exports, 1, p));
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
  initModuleData(env, md);
  
  // Expose and return the bindings this addon provides.
  return bindings(env, exports, md);
}
