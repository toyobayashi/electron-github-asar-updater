const path = require('path')
const fs = require('fs-extra')
const semver = require('semver')
// const request = require('request')
const got = require('got')
const { unzip } = require('zauz')
const download = require('./lib/download.js')
const updaterScript = require('./lib/updater.js')

let app = null
let isElectronEnvironment = true
try {
  app = require('electron').app || require('electron').remote.app
  if (typeof app.isPackaged === 'undefined') {
    Object.defineProperty(app, 'isPackaged', {
      get () {
        const execFile = path.basename(process.execPath).toLowerCase()
        if (process.platform === 'win32') {
          return execFile !== 'electron.exe'
        }
        return execFile !== 'electron'
      }
    })
  }
} catch (_) {
  isElectronEnvironment = false
  app = {
    relaunch () {
      require('child_process').spawn(process.argv0, process.argv.slice(1), { detached: process.platform === 'win32', stdio: 'ignore' }).unref()
    },
    exit (number) {
      process.exit(number)
    },
    getVersion () {
      try {
        return require(path.join(process.cwd(), 'package.json')).version
      } catch (__) {
        return '0.0.0'
      }
    }
  }
  Object.defineProperty(app, 'isPackaged', {
    get () {
      return false
    }
  })
}

const dotPatch = getPath('.patch')
const updater = getPath('updater')

class Updater {
  constructor (repo, prefix = 'app') {
    if (typeof repo !== 'string') throw new TypeError('Argument type error: new Updater(repo: string, prefix?: string)')
    if (typeof prefix !== 'string') throw new TypeError('Argument type error: new Updater(repo: string, prefix?: string)')
    this.repo = repo
    this.info = null
    this.reqObj = null
    this.prefix = prefix

    if (app.isPackaged) {
      if (!fs.existsSync(getPath('./updater/index.js')) || !fs.existsSync(getPath('./updater/package.json'))) {
        fs.mkdirsSync(updater)
        fs.writeFileSync(getPath('./updater/index.js'), updaterScript)
        fs.writeFileSync(getPath('./updater/package.json'), JSON.stringify({ main: './index.js' }))
      }

      if (fs.existsSync(dotPatch)) {
        this.relaunch()
      }
    }
  }

  relaunch () {
    if (app.isPackaged) {
      if (fs.existsSync(updater)) fs.renameSync(updater, getPath('app'))
    }
    app.relaunch()
    app.exit()
  }

  isReadyToDownload () {
    return !!this.info && !!this.info.appZipUrl
  }

  getUpdateInfo () {
    return this.info
  }

  abort () {
    if (this.reqObj) {
      this.reqObj.req.abort()
      this.reqObj.req = null
    }
  }

  download (onProgress) {
    if (app.isPackaged) {
      if (!this.isReadyToDownload()) return Promise.reject(`No update or target file \`${this.prefix}-\${platform}-\${arch}.zip\` not found.`)
      this.abort()
      return new Promise((resolve, reject) => {
        this.reqObj = download(this.info.appZipUrl, getPath('app.zip'), onProgress, (err, filepath) => {
          if (err) {
            reject(err)
            return
          }
          if (filepath) {
            if (!fs.existsSync(dotPatch)) fs.mkdirsSync(dotPatch)
            unzip(getPath('app.zip'), dotPatch).then(() => {
              fs.removeSync(getPath('app.zip'))
              resolve(true)
            }).catch(err => {
              reject(err)
            })
          } else {
            resolve(false)
          }
        })
      })
    } else {
      return Promise.resolve(false)
    }
  }

  check (options) {

    if (typeof options === 'undefined') {
      options = {
        prerelease: -1
      }
    } else if (Object.prototype.toString.call(options) === '[object Object]') {
      if (!options.hasOwnProperty('prerelease')) {
        options.prerelease = -1
      } else if (options.prerelease !== -1 && options.prerelease !== 0 && options.prerelease !== 1) {
        throw new TypeError('Argument type error: Updater.prototype.check(options?: { prerelease?: -1 | 0 | 1 })')
      }
    } else {
      throw new TypeError('Argument type error: Updater.prototype.check(options?: { prerelease?: -1 | 0 | 1 })')
    }

    this.info = null
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

    return Promise.all([
      got.get(releases.url, { json: true, headers }),
      got.get(tags.url, { json: true, headers })
    ]).then(([{ body: releaseList }, { body: tagList }]) => {
      releaseList = releaseList.filter(r => r.draft === false)

      if (options.prerelease === -1) {
        releaseList = releaseList.filter(r => r.prerelease === false)
      } else if (options.prerelease === 1) {
        releaseList = releaseList.filter(r => r.prerelease === true)
      }

      if (!releaseList.length) {
        this.info = null
        return null
      }

      releaseList.sort((a, b) => new Date(b.published_at).getTime() - new Date(a.published_at).getTime())

      const latest = releaseList[0]
      const version = latest.tag_name.substr(1)
      if (semver.gte(app.getVersion(), version)) {
        this.info = null
        return null
      }

      const appZip = latest.assets.filter((a) => a.name === `${this.prefix}-${process.platform}-${process.arch}.zip`)[0]
      const zip = latest.assets.filter((a) => ((a.content_type === 'application/x-zip-compressed' || a.content_type === 'application/zip') && (a.name.indexOf(`${process.platform}-${process.arch}`) !== -1)))[0]
      const exe = latest.assets.filter((a) => ((a.content_type === 'application/x-msdownload') && (a.name.indexOf(`${process.platform}-${process.arch}`) !== -1)))[0]

      const zipUrl = zip ? zip.browser_download_url : null
      const exeUrl = exe ? exe.browser_download_url : null
      const appZipUrl = appZip ? appZip.browser_download_url : null

      const latestTag = tagList.filter((tag) => tag.name === latest.tag_name)[0]
      const commit = latestTag ? latestTag.commit.sha : ''
      const info = { version, commit, zipUrl, exeUrl, appZipUrl, release: latest, tag: latestTag }
      this.info = info
      return info
    })
  }
}

function getPath (...relative) {
  return isElectronEnvironment ? path.join(process.resourcesPath, ...relative) : path.join(__dirname, '../../..', ...relative)
}

// function requestPromise (options) {
//   return new Promise((resolve, reject) => {
//     request(options, (err, res, body) => {
//       if (err) {
//         err.res = res
//         reject(err)
//       } else resolve(body)
//     })
//   })
// }

module.exports = Updater
