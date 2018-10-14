'use strict'

const B3 = require('./b3')

handleBackpressure(() => console.log('done'))

function handleBackpressure (done) {
  var b3 = b3common('handleBackpressure', 20, done)
  var count = 8
  b3.open()
  while (count--) b3.b2lrProducer.send(`+${B3.timeMs()} ms handleBackpressure`)
  setTimeout(() => b3.close(), 50)
}

function b3common (functionName, timeoutMs, done) {
  var b3 = new B3({ noSelfTest: true, noDefaultListeners: true })
  var count = 2
  b3.b2lrConsumer.on('token', t => {
    setTimeout(() => {
      b3.b2rlProducer.send(`+${B3.timeMs()} ms echo "${t.message}" back`)
      b3.b2lrConsumer.doneWith(t)
    }, timeoutMs)
  })
  b3.b2rlConsumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      B3.timeMs(), b3.b2rlConsumer.sid, t.sid, t.message, t.delay)
    b3.b2rlConsumer.doneWith(t)
  })
  b3.b2lrConsumer.on('close', () => {
    if (--count) return
    console.log(`+${B3.timeMs()} ms ${functionName} done`)
    done()
  })
  b3.b2rlConsumer.on('close', () => {
    if (--count) return
    console.log(`+${B3.timeMs()} ms ${functionName} done`)
    done()
  })
  return b3
}
