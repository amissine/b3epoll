'use strict'

const B2 = require('bindings')('b2')
var b3Count = 0
var start = Date.now()

class B3 {
  constructor (left, right) {
    this.b2lr = B2.newB2(left, right)
    this.b2rl = B2.newB2(right, left)
    this.b2lrProducer = this.b2lr.producer
    this.b2rlProducer = this.b2rl.producer
    var b2lrConsumer = this.b2lr.consumer
    var b2rlConsumer = this.b2rl.consumer
    this.count = b3Count++

    console.log('B3 this.count: %d', this.count)
    addListeners(b2lrConsumer)
    addListeners(b2rlConsumer)
  }
  open () {
    console.log('B3.open this.count: %d', this.count)
    this.b2lr.open()
    this.b2rl.open()

    selfTest(this.b2lrProducer)
    selfTest(this.b2rlProducer)
  }
  close () {
    console.log('B3.close this.count: %d', this.count)
    this.b2lr.close()
    this.b2rl.close()
  }
}
module.exports = B3

function addListeners (consumer) {
  var consumerSid = consumer.sid
  console.log('+%d ms - addListeners consumer.sid: %d',
    Date.now() - start, consumerSid)
  consumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      Date.now() - start, consumerSid, t.sid, t.message, t.delay)
    consumer.doneWith(t)
  })
  consumer.on('close', () => {
    console.log('+%d ms - sid %d threads are stopped now.',
      Date.now() - start, consumerSid)
  })
}

function selfTest (producer) {
  console.log('+%d ms - selfTest producer.sid: %d',
    Date.now() - start, producer.sid)
  producer.send('- selfTest message sent on ' + new Date())
}
