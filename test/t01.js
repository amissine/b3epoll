/* eslint-env mocha */

'use strict'

const assert = require('assert-plus')
const B3 = require('../')
var b3 = new B3()

describe('Basic B3 functionality', () => {
  it('does not require before(...) to get instantiated', done => {
    assert.object(b3, 'b3')
    assert.ok(b3.done(), 'b3.done()')
    done()
  })
  it('allows exactly one singleton instance of itself', done => {
    assert.throws(() => { var x = new B3(); return x }, 'Must throw Error')
    done()
  })
})
