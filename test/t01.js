/* eslint-env mocha */

'use strict'

const assert = require('assert-plus')
const B3 = require('../')
var b3 = new B3()

describe('Basic B3 functionality:', () => {
  it('does not require before(...) to get instantiated', done => {
    assert.object(b3, 'b3')
    done()
  })
  it('allows exactly one singleton instance of itself', done => {
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
})
