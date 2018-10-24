/* eslint-env mocha */

'use strict'

const assert = require('assert-plus')
const B3 = require('../b3')
var b3 = new B3()
var x
var count = 1

describe('A B3 module:', () => {
  it('is a bidirectional B2 (bounded buffer)', function (done) {
    if (count++) {
      assert.object(b3.l2rProducer, 'b3.l2rProducer')
      assert.object(b3.l2rConsumer, 'b3.l2rConsumer')
      assert.object(b3.r2lProducer, 'b3.r2lProducer')
      assert.object(b3.r2lConsumer, 'b3.r2lConsumer')
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
      done()
    }, 40)
  }).timeout(200)
  it('runs the echo exchange', done => runEchoExchange(done)
  ).timeout(200)
  it('handles the backpressure nicely', done => handleBackpressure(done)
  ).timeout(200)
  it('writes a file on disk with libuv', done => libuvWriteFile(done)
  ).timeout(200)
  it('copies the file with libuv', done => libuvCopyFile(done)
  ).timeout(200)
  it('copies the file with epoll', function (done) {
    if (process.platform === 'linux') epollCopyFile(done)
    else this.skip()
  }).timeout(200)
})

function epollCopyFile (done) {
  done()
}

function libuvCopyFile (done) {
  done()
}

function libuvWriteFile (done) {
  var b3 = new B3(
    B3.randomDataGenerator, // l2rProducer
    B3.libuvFileWriter, // l2rConsumer
    B3.customLrRlNotifier, // r2lProducer
    B3.defaults, // r2lProducer
    256, // l2rBufsize
    2, // r2lBufsize
    '/tmp/bigfile.t02' // l2rData
  )
  var notDone = true

  b3.r2lConsumer.on('token', t => {
    console.log('+%d ms consumer sid %d, token sid %d, message "%s", delay %d µs',
      B3.timeMs(), b3.r2lConsumer.sid, t.sid, t.message, t.delay)
    b3.r2lConsumer.doneWith(t)
    if (notDone) {
      b3.close()
      done()
      notDone = false
    }
  })
  b3.open()
}

function runEchoExchange (done) {
  var b3 = b3common('runEchoExchange', 1)
  b3.open()
  b3.l2rProducer.send(`+${B3.timeMs()} ms runEchoExchange started`)
  setTimeout(() => { b3.close(); b3.isClosed = true; done() }, 50)
}

function handleBackpressure (done) {
  var b3 = b3common('handleBackpressure', 20)
  var t = 8
  b3.open()
  while (t--) b3.l2rProducer.send(`+${B3.timeMs()} ms handleBackpressure`)
  setTimeout(() => {
    b3.close()
    b3.isClosed = true
    done()
  }, 50)
}

function b3common (functionName, timeoutMs) {
  var b3 = new B3(0, 0, 0, 0, 4, 4, '', '', true, true)
  b3.l2rConsumer.on('token', t => {
    // console.log('+%d ms consumer sid %d, token sid %d message %s, delay %d µs',
    //   B3.timeMs(), b3.b2lrConsumer.sid, t.sid, t.message, t.delay)
    if (b3.isClosed) {
      b3.l2rConsumer.doneWith(t)
      return
    }
    setTimeout(() => {
      // console.log('+%d ms consumer sid %d, token sid %d message %s, delay %d µs',
      //   B3.timeMs(), b3.b2lrConsumer.sid, t.sid, t.message, t.delay)
      if (b3.isClosed) {
        b3.l2rConsumer.doneWith(t)
        return
      }
      b3.r2lProducer.send(`+${B3.timeMs()} ms echo "${t.message}" back`)
      b3.l2rConsumer.doneWith(t)
    }, timeoutMs)
  })
  b3.r2lConsumer.on('token', t => {
    // console.log('+%d ms consumer sid %d, token sid %d, message %s, delay %d µs',
    //   B3.timeMs(), b3.b2rlConsumer.sid, t.sid, t.message, t.delay)
    b3.r2lConsumer.doneWith(t)
  })
  return b3
}
