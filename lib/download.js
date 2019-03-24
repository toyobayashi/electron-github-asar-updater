const path = require('path')
const fs = require('fs-extra')
const request = require('request')

module.exports = function download (u, p, onData, callback) {
  let isCall = false
  let req = null

  fs.mkdirsSync(path.dirname(p))
  if (fs.existsSync(p)) {
    if (!isCall && typeof callback === 'function') {
      callback(null, p)
      isCall = true
    }
    return
  }

  const headers = {
    'User-Agent': 'electron-github-asar-updater'
  }
  let fileLength = 0
  if (fs.existsSync(p + '.tmp')) {
    fileLength = fs.statSync(p + '.tmp').size
    if (fileLength > 0) {
      headers.Range = 'bytes=' + fileLength + '-'
    }
  }

  let rename = true
  let size = 0
  req = request.get({
    url: u,
    headers: headers,
    encoding: null
  })
  req.on('response', (res) => {
    if (res.statusCode >= 200 && res.statusCode < 400) {
      const contentLength = Number(res.headers['content-length'])
      let start = Date.now()
      req.on('data', (chunk) => {
        size += chunk.length
        if (typeof onData === 'function') {
          if (fileLength + size === fileLength + contentLength) {
            onData({
              name: path.parse(p).base,
              current: fileLength + size,
              max: fileLength + contentLength,
              loading: 100 * (fileLength + size) / (fileLength + contentLength)
            })
          } else {
            const now = Date.now()
            if (now - start > 16) {
              start = now
              onData({
                name: path.parse(p).base,
                current: fileLength + size,
                max: fileLength + contentLength,
                loading: 100 * (fileLength + size) / (fileLength + contentLength)
              })
            }
          }
        }
      })

      req.pipe(fs.createWriteStream(p + '.tmp', { flags: 'a+' }).on('close', () => {
        if (rename) {
          fs.renameSync(p + '.tmp', p)
          if (!isCall && typeof callback === 'function') {
            callback(null, p)
            isCall = true
          }
        }
      }).on('error', (err) => {
        if (err) {
          rename = false
          if (!isCall && typeof callback === 'function') {
            callback(err, '')
            isCall = true
          }
        }
      }))
    } else {
      if (!isCall && typeof callback === 'function') {
        callback(new Error(res.statusCode), '')
        isCall = true
      }
    }
  })

  req.on('abort', () => {
    rename = false
    if (!isCall && typeof callback === 'function') {
      callback(null, '')
      isCall = true
    }
  })
  req.on('error', err => {
    rename = false
    if (!isCall && typeof callback === 'function') {
      callback(err, '')
      isCall = true
    }
  })

  return req
}
