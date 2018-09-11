'use strict'

// const addon = require('bindings')('addon')
var b3Count = 0

class B3 {
  constructor () {
    if (b3Count++) {
      throw new Error('Multiple instances of B3 are not allowed')
    }
  }
  done () {
    return true
  }
}
module.exports = B3
