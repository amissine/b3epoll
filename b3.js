'use strict'

const B2 = require('bindings')('b2')
var start = Date.now()

/**
 * Bidirectional Bounded Buffer with N-API addon running producer/consumer threads
 * in each direction.
 */
class B3 {
  /**
   * @param {uint} l2rProducer - producer in the left to right direction
   * @param {uint} l2rConsumer - consumer in the left to right direction
   * @param {uint} r2lProducer - producer in the right to left direction
   * @param {uint} r2lConsumer - consumer in the right to left direction
   * @param {uint} l2rBufsize - the shared buffer size, 2^n instances
   *   of TokenType (see b2/b2.h), in the left to right direction
   * @param {uint} r2lBufsize - the shared buffer size, 2^n instances
   *   of TokenType (see b2/b2.h), in the right to left direction
   * @param {utf8} l2rData - data string to use in l2r b2 instance
   * @param {utf8} r2lData - data string to use in r2l b2 instance
   * @param {bool} noDefaultListeners - the caller will provide all the listeners
   * @param {bool} noSelfTest - the caller will be sending all the messages
   */
  constructor (
    l2rProducer = 0, // B3.defaults
    l2rConsumer = 0, // B3.defaults
    r2lProducer = 0, // B3.defaults
    r2lConsumer = 0, // B3.defaults
    l2rBufsize = 16, // 2^n
    r2lBufsize = 0, // l2rBufsize by default
    l2rData = '',
    r2lData = '',
    noDefaultListeners = false,
    noSelfTest = false
  ) {
    r2lBufsize = !r2lBufsize ? l2rBufsize : r2lBufsize
    this.noDefaultListeners =
      l2rProducer || l2rConsumer || r2lProducer || r2lConsumer ? true
        : noDefaultListeners
    this.noSelfTest =
      l2rProducer || l2rConsumer || r2lProducer || r2lConsumer ? true
        : noSelfTest

    this._l2r = B2.newB2(l2rProducer, l2rConsumer, l2rData, l2rBufsize)
    this._r2l = B2.newB2(r2lProducer, r2lConsumer, r2lData, r2lBufsize)
    this.l2rProducer = this._l2r.producer
    this.r2lProducer = this._r2l.producer
    this.l2rConsumer = this._l2r.consumer
    this.r2lConsumer = this._r2l.consumer
    if (this.noDefaultListeners) return

    addDefaultListener(this, this.l2rConsumer)
    addDefaultListener(this, this.r2lConsumer)
  }
  open () {
    this._l2r.open()
    this._r2l.open()
    if (this.noSelfTest) return

    selfTest(this.l2rProducer)
    selfTest(this.r2lProducer)
  }
  close () {
    this._l2r.close()
    this._r2l.close()
  }
  static timeMs () {
    return Date.now() - start
  }
}
B3.defaults = 0 // producer and consumer Ids
B3.sidSetter = 1 // producerId
B3.bioFileReader = 2 // producerId
B3.epollFileReader = 3 // producerId
B3.bioFileWriter = 1 // consumerId
B3.epollFileWriter = 2 // consumerId

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
