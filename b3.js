'use strict'

const B2 = require('bindings')('b2')
var b2lr
var b2rl
var b3Count = 0

class B3 {
  constructor (left, right) {
    b2lr = B2.newB2(left, right)
    b2rl = B2.newB2(right, left)
    b3Count++

    console.log('%d %O', b3Count, b2rl) // for lint
    b2lr.consumer.on('token', t => {
      console.log('%O', t)
      this.doneWith(t)
    })
  }
}
module.exports = B3
