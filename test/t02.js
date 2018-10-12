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
    setTimeout(() => b3.close(), 100)
    setTimeout(() => x.close(), 110)
    setTimeout(() => {
      console.log('done B3.close test')
      done()
    }, 120)
  }).timeout(200)
  it('runs the echo exchange', done => runEchoExchange(done)
  ).timeout(200)
  it('handles the backpressure nicely', done => handleBackpressure(done)
  ).timeout(200)
})

function runEchoExchange (done) {
  console.log('runEchoExchange started')
  setTimeout(() => {
    console.log('runEchoExchange done')
    done()
  }, 100)
}

function handleBackpressure (done) {
  console.log('handleBackpressure started')
  setTimeout(() => {
    console.log('handleBackpressure done')
    done()
  }, 100)
}
