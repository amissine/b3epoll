/* eslint-env mocha */

'use strict'

const assert = require('assert-plus')
const B3 = require('../b3')
var b3 = new B3()
var x
var count = 1

describe('Basic B3 functionality:', () => {
  it('is bidirectional', function (done) {
    if (count++) {
      assert.object(b3, 'b3')
      done()
    } else this.skip()
  })
  it('allows multiple instances of itself', done => {
    x = new B3()
    assert.object(x, 'x')
    assert.object(b3, 'b3')
    done()
  })
  it('properly opens and runs the instances of itself', done => {
    b3.open()
    x.open()
    done()
  })
  it('properly closes and destroys the instances of itself', done => {
    setTimeout(() => b3.close(), 40)
    setTimeout(() => x.close(), 40)
    setTimeout(() => {
      console.log('done B3.close test')
      done()
    }, 50)
  }).timeout(200)
  it('runs the echo exchange', done => runEchoExchange(done)
  ).timeout(200)
  it('handles the backpressure nicely', done => handleBackpressure(done)
  ).timeout(200)
})

function runEchoExchange (done) {
  var b3 = b3common('runEchoExchange', 1, done)
  b3.open()
  b3.b2lrProducer.send(`+${B3.timeMs()} ms runEchoExchange started`)
  setTimeout(() => b3.close(), 50)
}

function handleBackpressure (done) {
  var b3 = b3common('handleBackpressure', 20, done)
  var count = 8
  b3.open()
  while (count--) b3.b2lrProducer.send(`+${B3.timeMs()} ms handleBackpressure`)
  setTimeout(() => b3.close(), 50)
}

function b3common (functionName, timeoutMs, done) {
  var b3 = new B3({ noSelfTest: true, noDefaultListeners: true })
  var count = 2
  b3.b2lrConsumer.on('token', t => {
    setTimeout(() => {
      if (!b3.isOpen) return
      b3.b2rlProducer.send(`+${B3.timeMs()} ms echo "${t.message}" back`)
      b3.b2lrConsumer.doneWith(t)
    }, timeoutMs)
  })
  b3.b2rlConsumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      B3.timeMs(), b3.b2rlConsumer.sid, t.sid, t.message, t.delay)
    b3.b2rlConsumer.doneWith(t)
  })
  b3.b2lrConsumer.on('close', () => {
    if (--count) return
    console.log(`+${B3.timeMs()} ms ${functionName} done`)
    done()
  })
  b3.b2rlConsumer.on('close', () => {
    if (--count) return
    console.log(`+${B3.timeMs()} ms ${functionName} done`)
    done()
  })
  return b3
}
