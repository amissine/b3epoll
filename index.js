'use strict'

const addon = require('bindings')('addon')
var b3Count = 0
var theItem
var options

class B3 {
  constructor () {
    if (b3Count++) {
      throw new Error('Multiple instances of B3 are not allowed')
    }
  }
  start (opts) {
    options = opts || { consumer: { delay: 100 } }
    console.log('options: %O', options)
    addon.start(onToken, onItem)
    return true
  }
  stop () {
    console.log('theItem.prime: %d', theItem.prime)
    addon.doneWith(theItem, false) // NO MORE items wanted
    return true
  }
}
module.exports = B3

function onToken (token) {
  setTimeout(
    () => {
      console.log('token.prime: %d, token.delay: %d µs', token.prime, token.delay)
      addon.doneWith(token, true) // MORE tokens wanted
    },
    options.consumer.delay
  )
}

function onItem (item) {
  theItem = item
  setTimeout(
    () => {
      // console.log('item.prime: %d', item.prime)
      addon.produceToken(item)
      addon.doneWith(item, true) // MORE items wanted
    },
    400
  )
}
