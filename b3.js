'use strict'

const B2 = require('bindings')('b2')
var b3Count = 0

class B3 {
  constructor (left, right) {
    var b2lr = B2.newB2(left, right)
    var b2rl = B2.newB2(right, left)
    var b2lrConsumer = b2lr.consumer
    b3Count++

    console.log('%d %O %O', b3Count, b2rl, b2lrConsumer) // b2rl for lint
    b2lrConsumer.on('token', t => {
      console.log('%O', t)
      b2lrConsumer.doneWith(t)
    })
    b2lrConsumer.on('close', () => {
      console.log('The b2lr threads are stopped now.')
    })
  }
}
module.exports = B3
