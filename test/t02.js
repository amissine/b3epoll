/* eslint-env mocha */

'use strict'

const assert = require('assert-plus')
const B3 = require('../b3')
var b3 = new B3()
var x
var count = 1

describe('Basic B3 functionality:', () => {
  it('is bidirectional B2 (bounded buffer)', function (done) {
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
    setTimeout(() => {
      b3.close()
      x.close()
      console.log('done B3.close test')
      done()
    }, 40)
  }).timeout(200)
  it('runs the echo exchange', done => runEchoExchange(done)
  ).timeout(200)
  it('handles the backpressure nicely', done => handleBackpressure(done)
  ).timeout(200)
})

function runEchoExchange (done) {
  var b3 = b3common('runEchoExchange', 1)
  b3.open()
  b3.b2lrProducer.send(`+${B3.timeMs()} ms runEchoExchange started`)
  setTimeout(() => { b3.close(); b3.isClosed = true; done() }, 50)
}

function handleBackpressure (done) {
  var b3 = b3common('handleBackpressure', 20)
  var t = 8
  b3.open()
  while (t--) b3.b2lrProducer.send(`+${B3.timeMs()} ms handleBackpressure`)
  setTimeout(() => {
    b3.close()
    b3.isClosed = true
    // console.log(`+${B3.timeMs()} ms b3.isClosed ${b3.isClosed}`)
    done()
  }, 50)
}

function b3common (functionName, timeoutMs) {
  var b3 = new B3({ noSelfTest: true, noDefaultListeners: true })
  b3.b2lrConsumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      B3.timeMs(), b3.b2lrConsumer.sid, t.sid, t.message, t.delay)
    if (b3.isClosed) {
      b3.b2lrConsumer.doneWith(t)
      console.log('b3.isClosed')
      return
    }
    setTimeout(() => {
      console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
        B3.timeMs(), b3.b2lrConsumer.sid, t.sid, t.message, t.delay)
      if (b3.isClosed) {
        b3.b2lrConsumer.doneWith(t)
        console.log('b3.isClosed 2')
        return
      }
      b3.b2rlProducer.send(`+${B3.timeMs()} ms echo "${t.message}" back`)
      b3.b2lrConsumer.doneWith(t)
    }, timeoutMs)
  })
  b3.b2rlConsumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      B3.timeMs(), b3.b2rlConsumer.sid, t.sid, t.message, t.delay)
    b3.b2rlConsumer.doneWith(t)
  })
  return b3
}
