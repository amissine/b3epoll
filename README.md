# b3epoll
Bidirectional Bounded Buffer with N-API and Linux Epoll, pure C implementation. Uses `NAPI_EXPERIMENTAL` features, requires `"node": ">=10.10"`. Implements an event-driven [N-API](https://nodejs.org/api/n-api.html) solution to the [Single Producer-Consumer Problem](http://www.dcs.ed.ac.uk/home/adamd/essays/ex1.html). Tries to [minimize the use of mutexes](https://en.wikipedia.org/wiki/Producer–consumer_problem#Without_semaphores_or_monitors) between the producer-consumer threads. Has been tested on Ubuntu 16.04 LTS. Also runs on macOS with epoll tests skipped.

## Contents

- [Introduction](#introduction)
- [Tests](#tests)
- [Demos](#demos)
- [Acknowledgements](#acknowledgements)

## Introduction

The JavaScript's popularity and versatility are huge and growing. The NodeJS-NPM ecosystem is nice and powerful. And the N-API part of NodeJS allows all kinds of communication between the C/C++ addons and the JavaScript side of your app. If you write the crucial parts of your app in C, you can speed it up significantly. For example, you can add producer-consumer threads to it and make better use of your machine's CPUs.

If your server runs Linux, you can improve the scalability of your app and further speed it up by using Linux [epoll](http://man7.org/linux/man-pages/man7/epoll.7.html) and [eventfd](http://man7.org/linux/man-pages/man2/eventfd.2.html) features. It is nice to have event-driven support on a kernel level, makes it possible to build competitive realtime apps for financial, cryptocurrency and other sectors where speed is paramount.

## Tests

First, clone and install the project with
```
git clone https://github.com/amissine/b3epoll.git
cd b3epoll
npm install
```
Then run the tests with `npm test`. Sample outputs:

- [Linux](https://docs.google.com/document/d/1geCoT0rSZLRaRomakzuT3a_Ap5CQSNsm1dvFc3788i4/)
- [MacOS](https://docs.google.com/document/d/1qPa1v0a50fqGsK-rsPlXdakWtlvL13NsnC7uc1yEyX4/)

## Demos

Run `npm run demos` for the list of available demos. Presently, there are none.

## Acknowledgements

- [N-API example](https://github.com/gabrielschulhof/abi-stable-node-addon-examples/tree/tsfn_round_trip/thread_safe_function_round_trip/node-api)
- [µSockets](https://github.com/uNetworking/uSockets)
- [epoll](https://github.com/fivdi/epoll)
