#include "b2.h"

static void producer (struct B2 * b2) {
  void (*pt) (TokenType* tt, struct B2 * b2) = b2->producer.produceToken;
  while (b2->isOpen) {
    if (b2->produceCount - b2->consumeCount == b2->sharedBuffer_size) break;
    pt(&b2->sharedBuffer[b2->produceCount % b2->sharedBuffer_size], b2);
    uv_mutex_lock(&b2->tokenProducedMutex);
    if (b2->produceCount++ - b2->consumeCount == 0) {
      uv_cond_signal(&b2->tokenProduced);
    }
    uv_mutex_unlock(&b2->tokenProducedMutex);
  }
}

void produceTokens (void* data) {
  struct B2 * b2 = (struct B2 *) data;
  
  (*b2->producer.initOnOpen)(b2);
  while (b2->isOpen) {
    producer(b2);
    if (b2->produceCount - b2->consumeCount == b2->sharedBuffer_size) {
#ifdef DEBUG_PRINTF
      printf("produceTokens sid %d, sharedBuffer full, produceCount %u\n", 
          b2->b2t_this.sid, b2->produceCount);
#endif
      uv_mutex_lock(&b2->tokenConsumedMutex);

      // sharedBuffer is full
      while (b2->isOpen &&
          b2->produceCount - b2->consumeCount == b2->sharedBuffer_size)
        uv_cond_wait(&b2->tokenConsumed, &b2->tokenConsumedMutex);
      uv_mutex_unlock(&b2->tokenConsumedMutex);
    }
  }
  (*b2->producer.cleanupOnClose)(b2);
}

static void consumer (struct B2 * b2) {
  void (*ct) (TokenType* tt, struct B2 * b2) = b2->consumer.consumeToken;
  while (b2->isOpen) {
    if (b2->produceCount == b2->consumeCount) // sharedBuffer is empty
      break;
    ct(&b2->sharedBuffer[b2->consumeCount % b2->sharedBuffer_size], b2);
    uv_mutex_lock(&b2->tokenConsumedMutex);
    if (b2->produceCount - b2->consumeCount++ == b2->sharedBuffer_size) {
      uv_cond_signal(&b2->tokenConsumed);
    }
    uv_mutex_unlock(&b2->tokenConsumedMutex);
  }
}

void consumeTokens (void* data) {
  struct B2 * b2 = (struct B2 *) data;

  (*b2->consumer.initOnOpen)(b2);
  while (b2->isOpen) {
    consumer(b2);
    if (b2->produceCount == b2->consumeCount) { // sharedBuffer is empty
#ifdef DEBUG_PRINTF
      printf("consumeTokens sid %d, sharedBuffer empty, consumeCount %u\n", 
          b2->b2t_this.sid, b2->consumeCount);
#endif
      uv_mutex_lock(&b2->tokenProducedMutex);
      while (b2->isOpen && b2->produceCount == b2->consumeCount)
        uv_cond_wait(&b2->tokenProduced, &b2->tokenProducedMutex);
      uv_mutex_unlock(&b2->tokenProducedMutex);
    }
  }
  (*b2->consumer.cleanupOnClose)(b2);
}
