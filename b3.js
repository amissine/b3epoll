'use strict'

const b2 = require('bindings')('b2')
var b2lr
var b2rl
var b3Count = 0

class B3 {
  constructor (left, right) {
    b2lr = b2.newB2(left, right)
    b2rl = b2.newB2(right, left)
    b3Count++
    console.log('%d %O %O', b3Count, b2lr, b2rl)
  }
}
module.exports = B3
