'use strict'

const B2 = require('bindings')('b2')
var b3Count = 0
var start = Date.now()

/**
 * Bidirectional Bounded Buffer with N-API addon running producer/consumer threads
 * in each direction.
 */
class B3 {
  /**
   * @param {Object} opts
   * @param {boolean} opts.noDefaultListeners - the caller will provide all the
   *                                            listeners
   * @param {boolean} opts.noSelfTest - the caller will be sending all the
   *                                    messages
   * @param {Object} left
   * @param {Object} right
   */
  constructor (opts = {
    noDefaultListeners: false,
    noSelfTest: false
  }, left, right) {
    this.noDefaultListeners = opts.noDefaultListeners || false
    this.noSelfTest = opts.noSelfTest || false

    this._b2lr = B2.newB2(left, right)
    this._b2rl = B2.newB2(right, left)
    this.b2lrProducer = this._b2lr.producer
    this.b2rlProducer = this._b2rl.producer
    this.b2lrConsumer = this._b2lr.consumer
    this.b2rlConsumer = this._b2rl.consumer
    this.count = b3Count++
    console.log('B3 this.count: %d', this.count)
    if (this.noDefaultListeners) return

    addDefaultListeners(this.b2lrConsumer)
    addDefaultListeners(this.b2rlConsumer)
  }
  open () {
    console.log('B3.open this.count: %d', this.count)
    this._b2lr.open()
    this._b2rl.open()
    if (this.noSelfTest) return

    selfTest(this.b2lrProducer)
    selfTest(this.b2rlProducer)
  }
  close () {
    console.log('B3.close this.count: %d', this.count)
    this._b2lr.close()
    this._b2rl.close()
  }
  static timeMs () {
    return Date.now() - start
  }
}
module.exports = B3

function addDefaultListeners (consumer) {
  var consumerSid = consumer.sid
  console.log('+%d ms - addDefaultListeners consumer.sid: %d',
    B3.timeMs(), consumerSid)
  consumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      B3.timeMs(), consumerSid, t.sid, t.message, t.delay)
    consumer.doneWith(t)
  })
  consumer.on('close', () => {
    console.log('+%d ms - sid %d threads are stopped now.',
      B3.timeMs(), consumerSid)
  })
}

function selfTest (producer) {
  console.log('+%d ms - selfTest producer.sid: %d',
    B3.timeMs(), producer.sid)
  producer.send('- selfTest message sent on ' + new Date())
}
