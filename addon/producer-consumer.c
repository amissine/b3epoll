#include "addon.h"

#define BUFFER_SIZE 32768

volatile unsigned int produceCount = 0, consumeCount = 0;
TokenType sharedBuffer[BUFFER_SIZE];

static void producer (AddonData* ad) {
  while (1) {
    if (produceCount - consumeCount == BUFFER_SIZE) break;
    produceToken(&sharedBuffer[produceCount % BUFFER_SIZE]);
    if (produceCount++ - consumeCount == 0) {
      uv_mutex_lock(&ad->tokenProducedMutex);
      uv_cond_signal(&ad->tokenProduced);
      uv_mutex_unlock(&ad->tokenProducedMutex);
    }
  }
}

static void consumer (AddonData* ad) {
  while (1) {
    if (produceCount - consumeCount == 0) break;
    consumeToken(&sharedBuffer[consumeCount % BUFFER_SIZE]);
    if (produceCount - consumeCount++ == BUFFER_SIZE) {
      uv_mutex_lock(&ad->tokenConsumedMutex);
      uv_cond_signal(&ad->tokenConsumed);
      uv_mutex_unlock(&ad->tokenConsumedMutex);
    }
  }
}

static void produceTokens (AddonData* ad) {
  while (1) {
    producer(ad);
    uv_mutex_lock(&ad->tokenConsumedMutex);
    while (produceCount - consumeCount == BUFFER_SIZE) // sharedBuffer is full 
      uv_cond_wait(&ad->tokenConsumed, &ad->tokenConsumedMutex);
    uv_mutex_unlock(&ad->tokenConsumedMutex);
  }
}

static void consumeTokens (AddonData* ad) {
  while (1) {
    consumer(ad);
    uv_mutex_lock(&ad->tokenProducedMutex);
    while (produceCount - consumeCount == 0) // sharedBuffer is empty
      uv_cond_wait(&ad->tokenProduced, &ad->tokenProducedMutex);
    uv_mutex_unlock(&ad->tokenProducedMutex);
  }
}
