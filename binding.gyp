{
  "targets": [
    {
      "target_name": "b2",
      "sources": [
        "./b2/b2.c", 
        "./b2/module.c" 
      ],
      "defines": [
        "B3_DEFAULTS=0",
        "B3_RANDOM_DATA_GENERATOR=1",
        "B3_LIBUV_FILE_WRITER=2",
        "B3_CUSTOM_LR_RL_NOTIFIER=3",
        "NAPI_EXPERIMENTAL"
      ]
    }
  ]
}
