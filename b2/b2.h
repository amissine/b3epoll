#ifndef B2_H
#define B2_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <node_api.h>
#include <sys/time.h>

struct fifo {
  struct fifo* in;
  struct fifo* out;
  unsigned int sid; // sequence id
  size_t size; // number of tokens in the queue
};
static inline struct fifo* fifoInit (struct fifo* q) {
  q->in = q; q->out = q; q->sid = 0; q->size = 0; return q;
}
static inline struct fifo* fifoIn (struct fifo* q, struct fifo* t) {
  t->in = q->in; t->out = q; q->in->out = t; q->in = t;
  t->sid = q->sid++; t->size = ++q->size; return q;
}
static inline struct fifo* fifoOut (struct fifo* q) {
  if (q->size == 0) return NULL;
  struct fifo* t = q->out; q->out = t->out; t->out->in = q;
  q->size--; return t;
}
static inline bool fifoEmpty (struct fifo* q) {
  return q->size == 0;
}
// For any given element t of the non-empty fifo queue q,
//
//   t->sid + 1 == t->out->sid AND
//   q->out->sid < t->sid if q->out != t AND
//   q->in->sid > t->sid if q->in != t
//

// The data in the shared buffer.
typedef struct {
  struct fifo tt_this;
  char theMessage[128];
  long long int theDelay;
} TokenType;
static inline void initTokenType (TokenType* tt, char* theMessage) {
  struct timeval timer_us;
  if (gettimeofday(&timer_us, NULL) == 0) {
    tt->theDelay = ((long long int) timer_us.tv_sec) * 1000000ll +
      (long long int) timer_us.tv_usec;
  }
  else tt->theDelay = -1ll;
  
  size_t i0 = sizeof(tt->theMessage) - 1;
  strncpy(tt->theMessage, theMessage, i0);
  tt->theMessage[i0] = '\0';
} 

// The data associated with an instance of the module. This takes the place of
// global static variables, while allowing multiple instances of the module to
// co-exist.
typedef struct {
  struct fifo b2instances; // of type struct B2
  napi_ref b2t_constructor; // B2Type
  napi_ref pt_constructor;  // ProducerType
  napi_ref ct_constructor;  // ConsumerType
  napi_ref tt_constructor; // RESTRICTION: even though this implementation supports
  // multiple b2 instances, they all must operate on the same TokenType.
} ModuleData;

struct Producer {
  struct fifo tokens2produce;
};

struct Consumer {
  napi_threadsafe_function onToken, onClose;
};

struct B2 {
  struct fifo b2t_this;
  ModuleData* md;
  uv_thread_t producerThread, consumerThread;
  uv_cond_t tokenProduced, tokenConsumed, tokenProducing, tokenConsuming;
  uv_mutex_t tokenProducedMutex, tokenConsumedMutex,
             tokenProducingMutex, tokenConsumingMutex;
  struct Producer producer;
  struct Consumer consumer;
  volatile bool isOpen;
  volatile unsigned int produceCount, consumeCount;
  size_t sharedBuffer_size;
  TokenType sharedBuffer[];
};

static inline bool is_undefined (napi_env env, napi_value v) {
  napi_valuetype result;
  assert(napi_ok == napi_typeof(env, v, &result));
  return result == napi_undefined;
}

static inline bool is_instanceof (napi_env env, napi_ref cons_ref, napi_value v) {
  bool validate;
  napi_value constructor;
  assert(napi_ok == napi_get_reference_value(env, cons_ref, &constructor));
  assert(napi_ok == napi_instanceof(env, v, constructor, &validate));
  return validate;
}

static inline void defObj_n_props (napi_env env, ModuleData* md, 
    const char* utf8ClassName, napi_callback Constructor, napi_ref* constructor, 
    size_t n, napi_property_descriptor* properties, // [n]
    char** utf8PropName, napi_callback* Getter, napi_callback* Method) {
  napi_property_descriptor* p = properties; memset(p, 0, n * sizeof(*p));
  size_t m = n;
  while (m--) {
    p->utf8name = *utf8PropName++;
    p->getter = *Getter++;
    p->method = *Method++;
    p->attributes = p->method ? napi_default : napi_enumerable;
    p++->data = md;
  }
  napi_value objType;
  assert(napi_ok == napi_define_class(env, utf8ClassName, NAPI_AUTO_LENGTH,
        Constructor, md, n, properties, &objType));
  assert(napi_ok == napi_create_reference(env, objType, 1, constructor));
}

static inline napi_value newInstance (napi_env env, napi_ref c, void* data, 
    napi_finalize finalize_cb, void* finalize_hint) {
  napi_value constructor, result;

  assert(napi_ok == napi_get_reference_value(env, c, &constructor));
  assert(napi_ok == napi_new_instance(env, constructor, 0, 0, &result));
  assert(napi_ok == napi_wrap(env, result, data, 
        finalize_cb, finalize_hint, 0));
  return result;
}

static inline struct B2 * newB2native (napi_env env, size_t argc, napi_value* argv, 
    ModuleData* md) {
  struct B2Config {
    size_t sharedBuffer_size;
  } b2Config;
  if (argc == 2 && is_undefined(env, *argv++) && is_undefined(env, *argv)) {
    printf("newB2native: no config data, using defaults");

    b2Config.sharedBuffer_size = 4; // must be 2^n
  }
  size_t b2size = sizeof(struct B2) +
    sizeof(TokenType) * b2Config.sharedBuffer_size;
  struct B2 * b2 = (struct B2 *)memset(malloc(b2size), 0, b2size);
  b2->sharedBuffer_size = b2Config.sharedBuffer_size;
  b2->md = md;
  fifoIn(&md->b2instances, &b2->b2t_this);
  assert(uv_mutex_init(&b2->tokenProducedMutex) == 0);
  assert(uv_mutex_init(&b2->tokenConsumedMutex) == 0);
  assert(uv_mutex_init(&b2->tokenProducingMutex) == 0);
  assert(uv_mutex_init(&b2->tokenConsumingMutex) == 0);
  assert(uv_cond_init(&b2->tokenProduced) == 0);
  assert(uv_cond_init(&b2->tokenConsumed) == 0);
  assert(uv_cond_init(&b2->tokenProducing) == 0);
  assert(uv_cond_init(&b2->tokenConsuming) == 0);

  printf("; b2->b2t_this.sid: %u\n", b2->b2t_this.sid);

  return b2;
}

void produceTokens (void*);
void consumeTokens (void*);

/*
typedef struct {
  uv_mutex_t check_status_mutex, tokenProducedMutex, tokenConsumedMutex;
#ifdef TOKEN_JAVASCRIPT
  uv_mutex_t tokenProducingMutex, tokenConsumingMutex;
  uv_cond_t tokenProducing, tokenConsuming;
  struct fifo queue; // tokens to produce
#endif // TOKEN_JAVASCRIPT  
  uv_cond_t tokenProduced, tokenConsumed;
  uv_thread_t the_thread, producerThread, consumerThread;
  napi_threadsafe_function tsfn, onToken;
  napi_ref thread_item_constructor;
  napi_ref token_type_constructor;
  volatile bool js_accepts;
} AddonData;

extern volatile unsigned int produceCount, consumeCount;

#ifdef TOKEN_JAVASCRIPT
// These definitions are used in the beginning of the development. The data
// are prime numbers accompanied with the time it took to get one. The both
// sides of the shared buffer are the JavaScript functions.

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

  // These two values must be protected by the mutex.
  bool call_has_returned;
  bool return_value;
} ThreadItem;

void CallJs(napi_env env, napi_value js_cb, void* context, void* data);
void CallJs_onToken(napi_env env, napi_value js_cb, void* context, void* data);
napi_value ThreadItemConstructor (napi_env env, napi_callback_info info);
napi_value TokenTypeConstructor (napi_env env, napi_callback_info info);
napi_value GetPrime (napi_env env, napi_callback_info info);
napi_value GetTokenPrime (napi_env env, napi_callback_info info);
napi_value GetTokenDelay (napi_env env, napi_callback_info info);
void PrimeThread (void* data); 
napi_value RegisterReturnValue (napi_env env, napi_callback_info info);
napi_value NotifyTokenProducer (napi_env env, napi_callback_info info);
napi_value Start2ThreadsTokenJavascript (AddonData* ad);

#endif // TOKEN_JAVASCRIPT
*/
#endif // B2_H
