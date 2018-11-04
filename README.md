# b3epoll
Bidirectional Bounded Buffer with N-API and Linux Epoll, pure C implementation. Uses `NAPI_EXPERIMENTAL` features, requires `"node": ">=10.10"`. Implements an event-driven [N-API](https://nodejs.org/api/n-api.html) solution to the [Single Producer-Consumer Problem](http://www.dcs.ed.ac.uk/home/adamd/essays/ex1.html). Tries to [minimize the use of mutexes](https://en.wikipedia.org/wiki/Producer–consumer_problem#Without_semaphores_or_monitors) between the producer-consumer threads. Has been tested on Ubuntu 16.04 LTS. Also runs on macOS with epoll tests skipped.

## Contents

- [Introduction](#introduction)
- [Tests](#tests)
- [Demos](#demos)
- [Acknowledgements](#acknowledgements)

## Introduction

The JavaScript's popularity and versatility are huge and growing. The NodeJS-NPM ecosystem is nice and powerful. And the N-API part of NodeJS allows all kinds of communication between the C/C++ addons and the JavaScript side of your app. If you write the crucial parts of your app in C, you can speed it up significantly. For example, you can add producer-consumer threads to it and make better use of your machine's CPUs.

If your server runs Linux, you can improve the scalability of your app and further speed it up by using Linux [epoll](http://man7.org/linux/man-pages/man7/epoll.7.html) and [eventfd](http://man7.org/linux/man-pages/man2/eventfd.2.html) features. It is nice to have event-driven support on a kernel level and build competitive real-time apps for financial, cryptocurrency and other sectors where speed is paramount.

This project is an example of such an app. On its JavaScript side (file `b3.js`), it has module B3 which creates two instances of module B2 to support bidirectionality. The B2 module (files `b2/*.*`) is implemented in C and exposes to the JavaScript side its Producer and Consumer objects, among other properties. The Producer and Consumer objects run on two separate threads and exchange Tokens using their Bounded Buffer. Different Producer/Consumer implementations can be requested from the JavaScript side. One of the available Consumer implementations calls the JavaScript-side function (which runs on the main thread) each time a token is consumed, and then waits until the JavaScript side is done with the token.

I would like to extend my sincere gratitude and appreciation to [Gabriel Schulhof](https://github.com/gabrielschulhof/abi-stable-node-addon-examples/tree/tsfn_round_trip/thread_safe_function_round_trip/node-api) for his wonderful example of a thread-safe function roundtrip - that example made this project possible. Thanks again, Gabriel!

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
