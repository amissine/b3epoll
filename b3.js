'use strict'

const B2 = require('bindings')('b2')
var start = Date.now()

/**
 * Bidirectional Bounded Buffer with N-API addon running producer/consumer threads
 * in each direction.
 */
class B3 {
  /**
   * @param {Object} opts
   * @param {unsigned int} opts.b2lrProducer - producer in the left to right
   *                                           direction
   * @param {unsigned int} opts.b2lrConsumer - consumer in the left to right
   *                                           direction
   * @param {unsigned int} opts.b2rlProducer - producer in the right to left
   *                                           direction
   * @param {unsigned int} opts.b2rlConsumer - consumer in the right to left
   *                                           direction
   * @param {boolean} opts.noDefaultListeners - the caller will provide all the
   *                                            listeners
   * @param {boolean} opts.noSelfTest - the caller will be sending all the
   *                                    messages
   * @param {unsigned int} opts.bufsize - the shared buffer size, 2^n instances
   *                                      of TokenType (see b2/b2.h)
   */
  constructor (opts = {
    b2lrProducer: 0, // B3.defaults
    b2lrConsumer: 0, // B3.defaults
    b2rlProducer: 0, // B3.defaults
    b2rlConsumer: 0, // B3.defaults
    noDefaultListeners: false,
    noSelfTest: false,
    bufsize: 16 // 2^n
  }) {
    this.noDefaultListeners = opts.noDefaultListeners || false
    this.noSelfTest = opts.noSelfTest || false

    this._b2lr = B2.newB2(opts.b2lrProducer, opts.b2lrConsumer, opts.bufsize)
    this._b2rl = B2.newB2(opts.b2rlProducer, opts.b2rlConsumer, opts.bufsize)
    this.b2lrProducer = this._b2lr.producer
    this.b2rlProducer = this._b2rl.producer
    this.b2lrConsumer = this._b2lr.consumer
    this.b2rlConsumer = this._b2rl.consumer
    if (this.noDefaultListeners) return

    addDefaultListener(this, this.b2lrConsumer)
    addDefaultListener(this, this.b2rlConsumer)
  }
  open () {
    this._b2lr.open()
    this._b2rl.open()
    if (this.noSelfTest) return

    selfTest(this.b2lrProducer)
    selfTest(this.b2rlProducer)
  }
  close () {
    this._b2lr.close()
    this._b2rl.close()
  }
  static timeMs () {
    return Date.now() - start
  }
}
B3.defaults = 0 // producer and consumer Ids
B3.randomDataGenerator = 1 // producerId
B3.libuvFileWriter = 1 // consumerId
B3.customLrRlNotifier = 2 // producerId

module.exports = B3

function addDefaultListener (that, consumer) {
  // var consumerSid = consumer.sid
  // console.log('+%d ms addDefaultListeners consumer.sid: %d',
  //   B3.timeMs(), consumerSid)
  consumer.on('token', t => {
    // console.log('+%d ms consumer sid %d, token sid %d, message %s, delay %d µs',
    //   B3.timeMs(), consumerSid, t.sid, t.message, t.delay)
    consumer.doneWith(t)
  })
}

function selfTest (producer) {
  // console.log('+%d ms - selfTest producer.sid: %d',
  //   B3.timeMs(), producer.sid)
  producer.send('- selfTest message sent on ' + new Date())
}
