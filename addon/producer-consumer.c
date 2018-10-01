#include "addon.h"

#define BUFFER_SIZE 4 // 32768

volatile unsigned int produceCount = 0, consumeCount = 0;
TokenType sharedBuffer[BUFFER_SIZE];

static void producer (AddonData* ad) {
  while (ad->js_accepts) {
    if (produceCount - consumeCount == BUFFER_SIZE) break;
    produceToken(&sharedBuffer[produceCount % BUFFER_SIZE], ad);
    if (produceCount++ - consumeCount == 0) {
      uv_mutex_lock(&ad->tokenProducedMutex);
      uv_cond_signal(&ad->tokenProduced);
      uv_mutex_unlock(&ad->tokenProducedMutex);
    }
  }
}

static void consumer (AddonData* ad) {
  while (ad->js_accepts) {
    if (produceCount - consumeCount == 0) break;
    consumeToken(&sharedBuffer[consumeCount % BUFFER_SIZE], ad);
    if (produceCount - consumeCount++ == BUFFER_SIZE) {
      uv_mutex_lock(&ad->tokenConsumedMutex);
      uv_cond_signal(&ad->tokenConsumed);
      uv_mutex_unlock(&ad->tokenConsumedMutex);
    }
  }
}

void produceTokens (void* data) {
  AddonData* ad = (AddonData*) data;
  while (ad->js_accepts) {
    producer(ad);
    if (produceCount - consumeCount == BUFFER_SIZE) {
      printf("produceTokens: sharedBuffer is full\n");
      uv_mutex_lock(&ad->tokenConsumedMutex);

      // shared Buffer is full
      while (ad->js_accepts && produceCount - consumeCount == BUFFER_SIZE)
        uv_cond_wait(&ad->tokenConsumed, &ad->tokenConsumedMutex);
      uv_mutex_unlock(&ad->tokenConsumedMutex);
    }
  }
  printf("produceTokens returning\n");
}

void consumeTokens (void* data) {
  AddonData* ad = (AddonData*) data;
  while (ad->js_accepts) {
    consumer(ad);
    if (produceCount == consumeCount) {
      printf("consumeTokens: sharedBuffer is empty\n");
      uv_mutex_lock(&ad->tokenProducedMutex);

      // sharedBuffer is empty
      while (ad->js_accepts && produceCount == consumeCount)
        uv_cond_wait(&ad->tokenProduced, &ad->tokenProducedMutex);
      uv_mutex_unlock(&ad->tokenProducedMutex);
    }
  }
  printf("consumeTokens returning\n");
}
