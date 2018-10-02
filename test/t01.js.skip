/* eslint-env mocha */

'use strict'

const assert = require('assert-plus')
const B3 = require('../')
var b3 = new B3()

describe('Basic B3 functionality:', () => {
  it('allows exactly one singleton instance of itself', done => {
    assert.object(b3, 'b3')
    assert.throws(() => { var x = new B3(); return x }, 'Must throw Error')
    done()
  })
  it('starts the addon with consumer and producer threads', done => {
    assert.ok(b3.start(), 'b3.start()')
    done()
  })
  it('stops the addon and cleans up', done => {
    setTimeout(() => {
      assert.ok(b3.stop(), 'b3.stop()')
      done()
    }, 5000)
  }).timeout(6000)
  it('runs again with backpressure of tokens to produce', done => {
    setTimeout(() => {
      assert.ok(b3.start({ consumer: { delay: 900 } }), 'b3.start(backpressure)')
      done()
    }, 6000)
  }).timeout(7000)
  it('frees the unproduced tokens when stopped', done => {
    setTimeout(() => {
      assert.ok(b3.stop(), 'b3.stop(backpressure)')
      done()
    }, 11000)
  }).timeout(12000)
})
