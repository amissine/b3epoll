# b3epoll
Bidirectional Bounded Buffer with N-API and Linux Epoll, pure C implementation

## Contents

- [Introduction](#introduction)
- [Tests](#tests)
- [Demos](#demos)
- [Acknowledgements](#acknowledgements)

## Introduction

This [N-API](https://nodejs.org/dist/latest-v8.x/docs/api/n-api.html#n_api_n_api) solution to the [Single Producer-Consumer Problem](http://www.dcs.ed.ac.uk/home/adamd/essays/ex1.html) is an [event-driven](http://man7.org/linux/man-pages/man2/eventfd.2.html) one that [does not use semaphores or monitors](https://en.wikipedia.org/wiki/Producer–consumer_problem#Without_semaphores_or_monitors). It has been tested on Ubuntu 16.04 LTS.

## Tests

Run `npm test` to test the solution.

## Demos

Run `npm run demos` for the list of available demos.

## Acknowledgements

- [N-API example](https://github.com/gabrielschulhof/abi-stable-node-addon-examples/thread_safe_function_round_trip/node-api)
- [µSockets](https://github.com/uNetworking/uSockets)
- [epoll](https://github.com/fivdi/epoll)
