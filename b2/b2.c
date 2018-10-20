#include "b2.h"

static void consumeToken (TokenType* tt, struct B2 * b2) {

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
  assert(napi_ok == napi_call_threadsafe_function(b2->consumer.onToken,
        tt, napi_tsfn_blocking));
  if (tt->theDelay != 0ll) {
#ifdef DEBUG_PRINTF
    printf("consumeToken sid %d, wait for the shared token sid %d to be consumed\n",
        b2->b2t_this.sid, tt->tt_this.sid);
#endif
    uv_mutex_lock(&b2->tokenConsumingMutex);

    // Wait for the token to be consumed
    while (/*b2->isOpen && */tt->theDelay != 0ll)
      uv_cond_wait(&b2->tokenConsuming, &b2->tokenConsumingMutex);
    uv_mutex_unlock(&b2->tokenConsumingMutex);
  }
#ifdef DEBUG_PRINTF
  printf("consumeToken sid %d, shared token sid %d consumed\n", 
      b2->b2t_this.sid, tt->tt_this.sid);
#endif
}

static void produceToken (TokenType* tt, struct B2 * b2) {
  struct fifo* t;
#ifdef DEBUG_PRINTF
  if (b2->isOpen && b2->producer.tokens2produce.size == 0)
    printf("produceToken sid %d, wait for a token from the main thread\n",
        b2->b2t_this.sid);
#endif
  uv_mutex_lock(&b2->tokenProducingMutex);

  if (b2->producer.tokens2produce.size > 0) {

    // Token(s) have been produced by the main thread,
    // remove the first token from the queue.
    t = fifoOut(&b2->producer.tokens2produce);
    if (t) { // if it's not NULL, copy it to the shared buffer and return
      memcpy(tt, t, sizeof(TokenType));
      free(t);
      uv_mutex_unlock(&b2->tokenProducingMutex);
#ifdef DEBUG_PRINTF
      printf("produceToken sid %d, token sid %d shared, returning 1\n", 
          b2->b2t_this.sid, tt->tt_this.sid);
#endif
      return;
    }
  }

  // Otherwise, wait for a token from the main thread.
  while (b2->isOpen && b2->producer.tokens2produce.size == 0)
    uv_cond_wait(&b2->tokenProducing, &b2->tokenProducingMutex);

  if (b2->isOpen) {
    t = fifoOut(&b2->producer.tokens2produce); // remove it from the queue,
    memcpy(tt, t, sizeof(TokenType)); // copy to the shared buffer and free it
    free(t);
  }
  uv_mutex_unlock(&b2->tokenProducingMutex);
#ifdef DEBUG_PRINTF
  printf("produceToken sid %d, token sid %d shared, returning 2\n", 
      b2->b2t_this.sid, tt->tt_this.sid);
#endif
}

static void producer (struct B2 * b2) {
  while (b2->isOpen) {
    if (b2->produceCount - b2->consumeCount == b2->sharedBuffer_size) break;
    produceToken(&b2->sharedBuffer[b2->produceCount % b2->sharedBuffer_size], b2);
    if (b2->produceCount++ - b2->consumeCount == 0) {
      uv_mutex_lock(&b2->tokenProducedMutex);
      uv_cond_signal(&b2->tokenProduced);
      uv_mutex_unlock(&b2->tokenProducedMutex);
    }
  }
}

static void consumer (struct B2 * b2) {
  while (b2->isOpen) {
    if (b2->produceCount - b2->consumeCount == 0) break;
    consumeToken(&b2->sharedBuffer[b2->consumeCount % b2->sharedBuffer_size], b2);
    if (b2->produceCount - b2->consumeCount++ == b2->sharedBuffer_size) {
      uv_mutex_lock(&b2->tokenConsumedMutex);
      uv_cond_signal(&b2->tokenConsumed);
      uv_mutex_unlock(&b2->tokenConsumedMutex);
    }
  }
}

void produceTokens (void* data) {
  struct B2 * b2 = (struct B2 *) data;
  while (b2->isOpen) {
    producer(b2);
    if (b2->produceCount - b2->consumeCount == b2->sharedBuffer_size) {
#ifdef DEBUG_PRINTF
      printf("produceTokens sid: %d, sharedBuffer is full\n", b2->b2t_this.sid);
#endif
      uv_mutex_lock(&b2->tokenConsumedMutex);

      // sharedBuffer is full
      while (b2->isOpen &&
          b2->produceCount - b2->consumeCount == b2->sharedBuffer_size)
        uv_cond_wait(&b2->tokenConsumed, &b2->tokenConsumedMutex);
      uv_mutex_unlock(&b2->tokenConsumedMutex);
    }
  }
#ifdef DEBUG_PRINTF
  printf("produceTokens sid %d, returning\n", b2->b2t_this.sid);
#endif
}

void consumeTokens (void* data) {
  struct B2 * b2 = (struct B2 *) data;
  while (b2->isOpen) {
    consumer(b2);
    if (b2->produceCount == b2->consumeCount) {
#ifdef DEBUG_PRINTF
      printf("consumeTokens sid: %d, sharedBuffer is empty\n", b2->b2t_this.sid);
#endif
      uv_mutex_lock(&b2->tokenProducedMutex);

      // sharedBuffer is empty
      while (b2->isOpen && b2->produceCount == b2->consumeCount)
        uv_cond_wait(&b2->tokenProduced, &b2->tokenProducedMutex);
      uv_mutex_unlock(&b2->tokenProducedMutex);
    }
  }
  assert(napi_ok == napi_release_threadsafe_function(
        b2->consumer.onToken, napi_tsfn_release));
#ifdef DEBUG_PRINTF
  printf("consumeTokens sid %d, returning\n", b2->b2t_this.sid);
#endif
}
