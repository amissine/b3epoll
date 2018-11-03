# b3epoll
Bidirectional Bounded Buffer with N-API and Linux Epoll, pure C implementation. Uses `NAPI_EXPERIMENTAL` features, requires `"node": ">=10.10"`.

## Contents

- [Introduction](#introduction)
- [Tests](#tests)
- [Demos](#demos)
- [Acknowledgements](#acknowledgements)

## Introduction

This [N-API](https://nodejs.org/api/n-api.html) solution to the [Single Producer-Consumer Problem](http://www.dcs.ed.ac.uk/home/adamd/essays/ex1.html) is an [event-driven](https://en.wikipedia.org/wiki/Epoll) one that [tries not to use semaphores or monitors](https://en.wikipedia.org/wiki/Producer–consumer_problem#Without_semaphores_or_monitors). It has been tested on Ubuntu 16.04 LTS. It also runs on macOS with epoll tests skipped.

## Tests

First, clone and install the project with
```
git clone https://github.com/amissine/b3epoll.git
cd b3epoll
npm install
```
Then run the tests with `npm test`.

## Demos

Run `npm run demos` for the list of available demos. Presently, there are none.

## Acknowledgements

- [N-API example](https://github.com/gabrielschulhof/abi-stable-node-addon-examples/tree/tsfn_round_trip/thread_safe_function_round_trip/node-api)
- [µSockets](https://github.com/uNetworking/uSockets)
- [epoll](https://github.com/fivdi/epoll)
