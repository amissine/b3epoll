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
  it('properly closes and destroys the instances of itself', done => {
    setTimeout(() => b3.close(), 1000)
    setTimeout(() => x.close(), 1100)
    setTimeout(() => {
      console.log('done B3 close test')
      done()
    }, 1200)
  }).timeout(2000)
})
