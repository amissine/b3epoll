'use strict'

const B3 = require('./b3')

handleBackpressure(() => console.log('done'))

function handleBackpressure (done) {
  var b3 = b3common('handleBackpressure', 20)
  var t = 8
  b3.open()
  while (t--) b3.b2lrProducer.send(`+${B3.timeMs()} ms handleBackpressure`)
  setTimeout(() => {
    b3.close()
    b3.isClosed = true
    // console.log(`+${B3.timeMs()} ms b3.isClosed ${b3.isClosed}`)
    done()
  }, 50)
}

function b3common (functionName, timeoutMs) {
  var b3 = new B3({ noSelfTest: true, noDefaultListeners: true })
  b3.b2lrConsumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      B3.timeMs(), b3.b2lrConsumer.sid, t.sid, t.message, t.delay)
    if (b3.isClosed) {
      b3.b2lrConsumer.doneWith(t)
      console.log('b3.isClosed')
      return
    }
    setTimeout(() => {
      console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
        B3.timeMs(), b3.b2lrConsumer.sid, t.sid, t.message, t.delay)
      if (b3.isClosed) {
        b3.b2lrConsumer.doneWith(t)
        console.log('b3.isClosed 2')
        return
      }
      b3.b2rlProducer.send(`+${B3.timeMs()} ms echo "${t.message}" back`)
      b3.b2lrConsumer.doneWith(t)
    }, timeoutMs)
  })
  b3.b2rlConsumer.on('token', t => {
    console.log('+%d ms - consumer sid %d, token sid %d, message %s, delay %d µs',
      B3.timeMs(), b3.b2rlConsumer.sid, t.sid, t.message, t.delay)
    b3.b2rlConsumer.doneWith(t)
  })
  return b3
}
