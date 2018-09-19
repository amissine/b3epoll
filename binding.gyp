{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ 
        "./addon/module.c", 
        "./addon/producer-consumer.c", 
        "./addon/token-javascript.c",
        "./addon/ThreadItemClass.cpp" 
      ],
      "defines": [
        "NAPI_EXPERIMENTAL",
        "TOKEN_JAVASCRIPT"
      ]
    }
  ]
}
