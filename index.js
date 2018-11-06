const path = require('path')
const fs = require('fs-extra')
const semver = require('semver')
const request = require('request')
const { unzip } = require('zauz')
const download = require('./lib/download.js')

let app = null
try {
  app = require('electron')
  if (typeof app.isPackaged === 'undefined') {
    app.isPackaged = (() => {
      const execFile = path.basename(process.execPath).toLowerCase()
      if (process.platform === 'win32') {
        return execFile !== 'electron.exe'
      }
      return execFile !== 'electron'
    })()
  }
} catch (_) {
  app = {
    relaunch () {
      require('child_process').spawn(process.argv0, process.argv.slice(1), { detached: process.platform === 'win32', stdio: 'ignore' }).unref()
    },
    exit (number) {
      process.exit(number)
    },
    getVersion () {
      return require('./package.json').version
    },
    isPackaged () {
      return false
    }
  }
}

const dotPatch = getPath('.patch')
const updater = getPath('updater')

class Updater {
  constructor (repo) {
    if (!repo) throw new Error('new Updater(\'Your github repo\')')
    this.repo = repo
    this.info = null

    if (app.isPackaged) {
      
      if (!fs.existsSync(updater) || !fs.existsSync(path.join(updater, 'index.js'))) {
        fs.mkdirsSync(updater)
        fs.writeFileSync(path.join(updater, 'index.js'),
`
const path = require('path')
const fs = process.versions.electron ? require('original-fs') : require('fs')

let app = null
try {
  app = require('electron')
} catch (_) {
  app = {
    relaunch () {
      require('child_process').spawn(process.argv0, process.argv.slice(1), { detached: process.platform === 'win32', stdio: 'ignore' }).unref()
    },
    exit (number) { process.exit(number) }
  }
}

function getPath (...relative) {
  return (process.versions.electron && process.type === 'browser') ? path.join(process.resourcesPath, ...relative) : path.join(__dirname, '..', ...relative)
}

function relaunch () {
  app.relaunch()
  app.exit()
}

function removeSync (p) {
  const stat = fs.statSync(p)
  if (stat.isDirectory()) {
    const ls = fs.readdirSync(p).map(file => path.join(p, file))
    for (let i = 0; i < ls.length; i++) {
      removeSync(ls[i])
    }
    fs.rmdirSync(p)
  } else {
    fs.unlinkSync(p)
  }
}

function removeApp () {
  if (fs.existsSync(getPath('app.asar'))) removeSync(getPath('app.asar'))
  if (fs.existsSync(getPath('app.asar.unpacked'))) removeSync(getPath('app.asar.unpacked'))
}

function main () {
  const dotPatch = getPath('.patch')
  if (fs.existsSync(dotPatch)) {
    removeApp()
    const ls = fs.readdirSync(dotPatch).map(file => path.join(p, file))
    for (let i = 0; i < ls.length; i++) {
      fs.renameSync(ls[i], getPath(path.basename(ls[i])))
    }
    removeSync(dotPatch)
  }
  fs.renameSync(getPath('app'), getPath('updater'))
  relaunch()
}

main()
`)
      }
      if (fs.existsSync(dotPatch)) {
        this.relaunch()
      }
    }
  }

  relaunch () {
    if (app.isPackaged) {
      fs.renameSync(getPath('updater'), getPath('app'))
    }
    app.relaunch()
    app.exit()
  }

  hasUpdate () {
    return !!this.info && !!this.info.appZipUrl
  }

  getUpdateInfo () {
    return this.info
  }

  download (onProgress) {
    if (app.isPackaged) {
      if (!this.hasUpdate()) return Promise.reject('No update.')
      return download(this.info.appZipUrl, getPath('app.zip'), onProgress).then(() => {
        if (!fs.existsSync(dotPatch)) fs.mkdirsSync(dotPatch)
        return unzip(getPath('app.zip'), dotPatch)
      }).then((size) => {
        fs.removeSync(getPath('app.zip'))
        return size
      })
    } else {
      return Promise.resolve(-1)
    }
  }

  check () {
    const headers = {
      'User-Agent': 'electron-github-asar-updater'
    }
    const releases = {
      url: `https://api.github.com/repos/${this.repo}/releases`,
      json: true,
      headers
    }
    const tags = {
      url: `https://api.github.com/repos/${this.repo}/tags`,
      json: true,
      headers
    }
    return new Promise((resolve, reject) => {
      request(releases, (err, _res, body) => {
        if (err) {
          reject(err)
          return
        }
  
        if (!body.length) {
          this.info = null
          resolve(null)
          return
        }
  
        const latest = body[0]
        const version = latest.tag_name.substr(1)
        if (semver.gte(app.getVersion(), version)) {
          this.info = null
          resolve(null)
          return
        }

        const appZip = latest.assets.filter((a) => a.name === `app-${process.platform}.zip`)[0]
        const zip = latest.assets.filter((a) => ((a.content_type === 'application/x-zip-compressed' || a.content_type === 'application/zip') && (a.name.indexOf(`${process.platform}-${process.arch}`) !== -1)))[0]
        const exe = latest.assets.filter((a) => ((a.content_type === 'application/x-msdownload') && (a.name.indexOf(`${process.platform}-${process.arch}`) !== -1)))[0]
  
        const zipUrl = zip ? zip.browser_download_url : null
        const exeUrl = exe ? exe.browser_download_url : null
        const appZipUrl = appZip ? appZip.browser_download_url : null
  
        request(tags, (err, _res, body) => {
          if (err) {
            reject(err)
            return
          }
  
          const latestTag = body.filter((tag) => tag.name === latest.tag_name)[0]
          const commit = latestTag.commit.sha
          const versionData = { version, commit, zipUrl, exeUrl, appZipUrl }
          this.info = versionData
          resolve(versionData)
        })
      })
    })
  }
}

function getPath (...relative) {
  return (process.versions.electron && process.type === 'browser') ? path.join(process.resourcesPath, ...relative) : path.join(__dirname, '../../..', ...relative)
}

module.exports = Updater
