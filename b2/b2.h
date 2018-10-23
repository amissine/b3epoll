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

struct B2;

struct Producer {
  struct fifo tokens2produce;
  void (*cleanupOnClose) (struct B2 *);
  void (*produceToken) (TokenType* tt, struct B2 * b2);
};

struct Consumer {
  napi_threadsafe_function onToken;
  void (*cleanupOnClose) (struct B2 *);
  void (*consumeToken) (TokenType* tt, struct B2 * b2);
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

static inline uint32_t uint32 (napi_env env, napi_value unsignedInt) {
  uint32_t result;
  assert(napi_ok == napi_get_value_uint32(env, unsignedInt, &result));
  return result;
}

void produceTokens (void*);
void consumeTokens (void*);

#endif // B2_H
