'use strict'

const addon = require('bindings')('addon')
var b3Count = 0
var theItem

class B3 {
  constructor () {
    if (b3Count++) {
      throw new Error('Multiple instances of B3 are not allowed')
    }
  }
  start () {
    addon.start(onItem)
    return true
  }
  stop () {
    console.log('theItem.prime: %d', theItem.prime)
    addon.doneWith(theItem, false) // NO MORE items wanted
    return true
  }
}
module.exports = B3

function onItem (item) {
  console.log('item.prime: %d', item.prime)
  theItem = item
  setTimeout(
    () => {
      addon.produceToken(item)
      addon.doneWith(item, true) // MORE items wanted
    },
    200
  )
}
