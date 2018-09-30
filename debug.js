'use strict'

const B3 = require('./')
const b3 = new B3()

b3.start()
setTimeout(() => b3.stop(), 5000)
