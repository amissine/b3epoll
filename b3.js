'use strict'

const B2 = require('bindings')('b2')
var b3Count = 0

class B3 {
  constructor (left, right) {
    this.b2lr = B2.newB2(left, right)
    this.b2rl = B2.newB2(right, left)
    var b2lrConsumer = this.b2lr.consumer
    var b2rlConsumer = this.b2rl.consumer
    this.count = b3Count++

    console.log('B3 this.count: %d', this.count)
    addListeners(b2lrConsumer)
    addListeners(b2rlConsumer)
  }
  open () {
    console.log('open B3 this.count: %d', this.count)
    this.b2lr.open()
    this.b2rl.open()
  }
  close () {
    console.log('close B3 this.count: %d', this.count)
    this.b2lr.close()
    this.b2rl.close()
  }
}
module.exports = B3

function addListeners (consumer) {
  console.log('addListeners consumer.sid: %d', consumer.sid)
  consumer.on('token', t => {
    console.log('%O', t)
    consumer.doneWith(t)
  })
  consumer.on('close', () => {
    console.log('consumer.sid %d threads are stopped now.', consumer.sid)
  })
}
