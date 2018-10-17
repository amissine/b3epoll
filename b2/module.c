#include "b2.h"

static void FreeModuleData (napi_env env, void* data, void* hint) {
#ifdef DEBUG_PRINTF
  printf("FreeModuleData started\n");
#endif
  ModuleData* md = (ModuleData*) data;
  assert(napi_ok == napi_delete_reference(env, md->b2t_constructor));
  assert(napi_ok == napi_delete_reference(env, md->pt_constructor));
  assert(napi_ok == napi_delete_reference(env, md->ct_constructor));
  assert(napi_ok == napi_delete_reference(env, md->tt_constructor));
  free(data);
}

// Constructor for instances of the `B2Type` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `B2Type`
// structure.
napi_value B2TypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value B2T_Open (napi_env env, napi_callback_info info) {
  napi_value this;
  ModuleData* md;
  struct B2 * b2;
 
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void*)&b2));

  // Reset the shared buffer.
  b2->produceCount = 0;
  b2->consumeCount = 0;
  b2->isOpen = 1;

  // Create and start the consumer thread.
  assert(uv_thread_create(&b2->consumerThread, consumeTokens, b2) == 0);

  // Create and start the producer thread.
  assert(uv_thread_create(&b2->producerThread, produceTokens, b2) == 0);

  return NULL;
}

static napi_value B2T_Close (napi_env env, napi_callback_info info) {
  napi_value this;
  ModuleData* md;
  struct B2 * b2;
 
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void*)&b2));

  // Stop the producer-consumer threads, destroy the uv threading harness.
  b2->isOpen = 0;
  uv_mutex_lock(&b2->tokenProducingMutex);
  uv_cond_signal(&b2->tokenProducing);
  uv_mutex_unlock(&b2->tokenProducingMutex);
  uv_mutex_lock(&b2->tokenConsumingMutex);
  uv_cond_signal(&b2->tokenConsuming);
  uv_mutex_unlock(&b2->tokenConsumingMutex);
  assert(uv_thread_join(&b2->producerThread) == 0);
  assert(uv_thread_join(&b2->consumerThread) == 0);
  uv_mutex_destroy(&b2->tokenProducedMutex); 
  uv_mutex_destroy(&b2->tokenConsumedMutex); 
  uv_mutex_destroy(&b2->tokenProducingMutex); 
  uv_mutex_destroy(&b2->tokenConsumingMutex); 
  uv_cond_destroy(&b2->tokenProduced); 
  uv_cond_destroy(&b2->tokenConsumed); 
  uv_cond_destroy(&b2->tokenProducing); 
  uv_cond_destroy(&b2->tokenConsuming);

  // Remove this b2 from md->b2instances, empty the queue of tokens that have not
  // been produced, free the unproduced tokens and the b2.
#ifdef DEBUG_PRINTF
  unsigned int sid = b2->b2t_this.sid;
#endif
  struct fifo * q = &b2->b2t_this, * queue = &md->b2instances;
  struct fifo * p = q->out, * r = q->in;
  p->in = r; r->out = p; queue->size--;
  while ((q = fifoOut(&b2->producer.tokens2produce))) {
#ifdef DEBUG_PRINTF
    printf("B2T_Close sid %u, unproduced token sid %u\n", sid, q->sid);
#endif
    free(q);
  }
  free(b2);
#ifdef DEBUG_PRINTF
  printf("B2T_Close sid %u, md->b2instances.size %zu\n",
      sid, md->b2instances.size);
#endif
  return NULL;
}

static napi_value B2T_Producer (napi_env env, napi_callback_info info) {
  napi_value this, producer;
  ModuleData* md;
  struct B2 * b2;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void*)&b2));
  producer = newInstance(env, md->pt_constructor, b2, 0, 0);
  return producer;
}

static napi_value B2T_Consumer (napi_env env, napi_callback_info info) {
  napi_value this, consumer;
  ModuleData* md;
  struct B2 * b2;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void*)&b2));
  consumer = newInstance(env, md->ct_constructor, b2, 0, 0);
  return consumer;
}

// Constructor for instances of the `ProducerType` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `ProducerType`
// structure.
napi_value ProducerTypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value PT_Send (napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv, this;
  ModuleData* md;
  struct B2 * b2;
  char msg[128];
  TokenType* tt;

  assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void*)&b2));
  assert(napi_ok == napi_get_value_string_utf8(env, argv, msg, 128, &argc));

  // Initialise the token with the item data, queue it and notify
  // the producer thread.
  tt = memset(malloc(sizeof(*tt)), 0, sizeof(*tt));
  initTokenType(tt, msg);
#ifdef DEBUG_PRINTF
  printf("PT_Send sid %d is about to queue tt->theMessage '%s'\n",
      b2->b2t_this.sid, tt->theMessage);
#endif
  uv_mutex_lock(&b2->tokenProducingMutex);
  fifoIn(&b2->producer.tokens2produce, &tt->tt_this);
  uv_cond_signal(&b2->tokenProducing);
  uv_mutex_unlock(&b2->tokenProducingMutex);
#ifdef DEBUG_PRINTF
  printf("PT_Send sid %d queued token sid %d\n",
      b2->b2t_this.sid, tt->tt_this.sid);
#endif
  return NULL;
}

// Constructor for instances of the `ConsumerType` class. This doesn't need to do
// anything since all we want the class for is to be able to type-check
// JavaScript objects that carry within them a pointer to a native `ConsumerType`
// structure.
napi_value ConsumerTypeConstructor (napi_env env, napi_callback_info info) {
  return NULL;
}

// Getter for the `sid` property of the `B2Type`, `ProducerType`, or
// `ConsumerType` object.
static napi_value GetSid (napi_env env, napi_callback_info info) {
  napi_value this, property;
  ModuleData* md;
  struct B2 * b2;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void**)&b2));
  assert(napi_ok == napi_create_uint32(env, b2->b2t_this.sid, &property));
  return property;
}

static void FinalizeOnClose (napi_env env, void* data, void* context) {
#ifdef DEBUG_PRINTF
  ModuleData* md = (ModuleData*)data;
  printf("FinalizeOnClose md->b2instances.size: %zu\n", 
      md->b2instances.size);
#endif
}

static void FinalizeOnToken (napi_env env, void* data, void* context) {
#ifdef DEBUG_PRINTF
  ModuleData* md = (ModuleData*)data;
  printf("FinalizeOnToken md->b2instances.size: %zu\n", 
      md->b2instances.size);
#endif
}

// This function is responsible for converting the native data coming in from
// the consumer thread to JavaScript values, and for calling the JavaScript
// function.
void CallJs_onToken(napi_env env, napi_value js_cb, void* context, void* data) {
  struct B2 * b2 = (struct B2 *)context;
  napi_value constructor;
  napi_value undefined, argv;

  // Retrieve the JavaScript `undefined` value. This will serve as the `this`
  // value for the function call.
  assert(napi_get_undefined(env, &undefined) == napi_ok);
    
  // Retrieve the constructor for the JavaScript class from which the item
  // holding the native data will be constructed.
  assert(napi_ok == napi_get_reference_value(
        env, b2->md->tt_constructor, &constructor));

  // Construct a new instance of the JavaScript class to hold the native item.
  assert(napi_ok == napi_new_instance(env, constructor, 0, 0, &argv));

  // Associate the native token with the newly constructed JavaScript object.
  assert(napi_ok == napi_wrap(env, argv, data, 0, 0, 0));

  // Call the JavaScript function with the token wrapped into an instance of
  // the JavaScript `TokenType` class.
  assert(napi_ok == napi_call_function(env, undefined, js_cb, 1, &argv, 0));
}

static napi_value CT_On (napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2], this, nameT, nameC;
  ModuleData* md;
  struct B2 * b2;
  char event[6], descT[] = "b2 token consumer", descC[] = "b2 stop callback";

  assert(napi_ok == napi_get_cb_info(env, info, &argc, argv, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void*)&b2));
  assert(napi_ok == napi_get_value_string_utf8(env, argv[0], event, 6, &argc));
  if (strcmp("token", event) == 0) { // create the onToken tsfn
    assert(napi_ok == napi_create_string_utf8(
          env, descT, NAPI_AUTO_LENGTH, &nameT));
    assert(napi_ok == napi_create_threadsafe_function(env, argv[1], 0, nameT,
          0, 1, md, FinalizeOnToken, b2, CallJs_onToken, &b2->consumer.onToken));
  }
  else { // create the onClose tsfn
    assert(napi_ok == napi_create_string_utf8(
          env, descC, NAPI_AUTO_LENGTH, &nameC));
    assert(napi_ok == napi_create_threadsafe_function(env, argv[1], 0, nameC,
          0, 1, md, FinalizeOnClose, b2, 0, &b2->consumer.onClose));
  }
  return NULL;
}

static napi_value CT_DoneWith (napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value this, argv;
  ModuleData* md;
  TokenType* tt;
  struct B2 * b2;

  assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, &this, (void*)&md));
  assert(napi_ok == napi_unwrap(env, this, (void*)&b2));

  // Retrieve the native token.
  assert(napi_ok == napi_unwrap(env, argv, (void**)&tt));

  // Notify the consumer thread that the token has been consumed.
  uv_mutex_lock(&b2->tokenConsumingMutex);
  tt->theDelay = 0ll;
  uv_cond_signal(&b2->tokenConsuming);
  uv_mutex_unlock(&b2->tokenConsumingMutex);
  
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
static napi_value TT_GetSid (napi_env env, napi_callback_info info) {
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
static napi_value TT_GetMessage (napi_env env, napi_callback_info info) {
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
static napi_value TT_GetDelay (napi_env env, napi_callback_info info) {
  napi_value jsthis, property;
  ModuleData* md;
  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&md));
  assert(is_instanceof(env, md->tt_constructor, jsthis));
  TokenType* token;
  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&token));
  assert(napi_ok == napi_create_int64(env, token->theDelay, &property));
  return property;
}

static inline void InitModuleData (napi_env env, ModuleData* md) {
  fifoInit(&md->b2instances);
 
  // Define the token type. The md->tt_constructor napi_ref will be deleted
  // during the 'FreeModuleData' call.
  char* propNamesTT[3] = { "sid", "message", "delay" };
  napi_property_descriptor pTT[3];
  napi_callback methodsTT[3] = { 0, 0, 0 },
               gettersTT[3] = { TT_GetSid, TT_GetMessage, TT_GetDelay }; 
  defObj_n_props(env, md, "TokenType", TokenTypeConstructor,
      &md->tt_constructor, 3, pTT, propNamesTT, gettersTT, methodsTT);

  // Define the bounded buffer type. The md->b2t_constructor napi_ref 
  // will be deleted during the 'FreeModuleData' call.
  char* propNamesB2T[5] = { "sid", "producer", "consumer", "open", "close" };
  napi_property_descriptor pB2T[5];
  napi_callback methodsB2T[5] = { 0, 0, 0, B2T_Open, B2T_Close },
                gettersB2T[5] = { GetSid, B2T_Producer, B2T_Consumer, 0, 0 };
  defObj_n_props(env, md, "B2Type", B2TypeConstructor,
      &md->b2t_constructor, 5, pB2T, propNamesB2T, gettersB2T, methodsB2T);

  // Define the producer type. The md->pt_constructor napi_ref will be deleted
  // during the 'FreeModuleData' call.
  char* propNamesPT[2] = { "sid", "send" };
  napi_property_descriptor pPT[2];
  napi_callback methodsPT[2] = { 0, PT_Send },
                gettersPT[2] = { GetSid, 0 };
  defObj_n_props(env, md, "ProducerType", ProducerTypeConstructor,
      &md->pt_constructor, 2, pPT, propNamesPT, gettersPT, methodsPT);

  // Define the consumer type. The md->ct_constructor napi_ref will be deleted
  // during the 'FreeModuleData' call.
  char* propNamesCT[3] = { "sid", "on", "doneWith" };
  napi_property_descriptor pCT[3];
  napi_callback methodsCT[3] = { 0, CT_On, CT_DoneWith },
                gettersCT[3] = { GetSid, 0, 0 };
  defObj_n_props(env, md, "ConsumerType", ConsumerTypeConstructor,
      &md->ct_constructor, 3, pCT, propNamesCT, gettersCT, methodsCT);
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
napi_value NewB2 (napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2], this;
  ModuleData* md;
  struct B2 *b2;

  assert(napi_ok == napi_get_cb_info(env, info, &argc, argv, 0, (void*)&md));
  b2 = newB2native(env, argc, argv, md);
  fifoInit(&b2->producer.tokens2produce);
  this = newInstance(env, md->b2t_constructor, b2, 0, 0);
  return this;
}

static inline napi_value Bindings (
    napi_env env, napi_value exports, ModuleData* md) {
  napi_property_descriptor p[] = {
    { "newB2", 0, NewB2, 0, 0, 0, napi_default, md }
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
  assert(napi_ok == napi_wrap(env, exports, md, FreeModuleData, 0, 0));
  InitModuleData(env, md);
  
  // Expose and return the bindings this addon provides.
  return Bindings(env, exports, md);
}
