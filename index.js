const path = require('path')
const fs = require('fs-extra')
const semver = require('semver')
const request = require('request')
const { unzip } = require('zauz')
const download = require('./lib/download.js')
const updaterScript = require('./lib/updater.js')

let app = null
try {
  app = require('electron').app
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

  check (options) {

    if (typeof options === 'undefined') {
      options = {
        prerelease: -1
      }
    } else if (Object.prototype.toString.call(options) === '[object Object]') {
      if (!options.hasOwnProperty('prerelease')) {
        options.prerelease = -1
      } else if (options.prerelease !== -1 && options.prerelease !== 0 && options.prerelease !== 1) {
        throw new Error('Argument type error: Updater.prototype.check(options?: { prerelease?: -1 | 0 | 1 })')
      }
    } else {
      throw new Error('Argument type error: Updater.prototype.check(options?: { prerelease?: -1 | 0 | 1 })')
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
      requestPromise(releases),
      requestPromise(tags)
    ]).then(([releaseList, tagList]) => {
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

      const appZip = latest.assets.filter((a) => a.name === `app-${process.platform}.zip`)[0]
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
  return (process.versions.electron && process.type === 'browser') ? path.join(process.resourcesPath, ...relative) : path.join(__dirname, '../../..', ...relative)
}

function requestPromise (options) {
  return new Promise((resolve, reject) => {
    request(options, (err, res, body) => {
      if (err) {
        err.res = res
        reject(err)
      } else resolve(body)
    })
  })
}

module.exports = Updater
