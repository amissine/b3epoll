'use strict'

// const b2 = require('bindings')('b2')
var b3Count = 0

class B3 {
  constructor (client, server) {
    /*
    b2.newB2(client, server)
    b2.newB2(server, client)
    */
    b3Count++
    console.log(b3Count)
  }
}
module.exports = B3
