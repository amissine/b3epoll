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
    var result = addon.start((item, thePrime) => {
      console.log('thePrime: %d', thePrime)
      theItem = item
    })
    console.log('start result: ' + result)
    return true
  }
  stop () {
    var result = addon.stop(theItem, false)
    console.log('stop result: ' + result)
    return true
  }
}
module.exports = B3
