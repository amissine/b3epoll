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
    addon.start((item, thePrime) => {
      console.log('thePrime: %d', thePrime)
      theItem = item
      setTimeout(() => addon.stop(item, true), 200)
    })
    return true
  }
  stop () {
    addon.stop(theItem, false)
    return true
  }
}
module.exports = B3
