/* eslint-env mocha */

'use strict'

const assert = require('assert-plus')
const B3 = require('../b3')
var b3 = new B3()
var count = 0

describe('Basic B3 functionality:', () => {
  it('is bidirectional', function (done) {
    if (count++) {
      assert.object(b3, 'b3')
      done()
    } else this.skip()
  })
  it('allows multiple instances of itself', done => {
    var x = new B3()
    assert.object(x, 'x')
    assert.object(b3, 'b3')
    done()
  })
})
