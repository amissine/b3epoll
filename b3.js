'use strict'

// const b2 = require('bindings')('b2')
var lrB2
var rlB2
var b3Count = 0

class B3 {
  constructor (leftBound, rightBound) {
    /*
    lrB2 = b2.newB2(leftBound, rightBound)
    rlB2 = b2.newB2(rightBound, leftBound)
    */
    b3Count++
    console.log(b3Count)
  }
}
module.exports = B3
