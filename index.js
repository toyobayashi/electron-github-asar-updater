const path = require('path')
const { spawn } = require('child_process')
const fs = require('fs')
const semver = require('semver')
const got = require('got').default
const { unzip } = require('zauz')
const ProxyAgent = require('proxy-agent')
const tybysDownloader = require('@tybys/downloader')
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
      spawn(process.argv0, process.argv.slice(1), { detached: process.platform === 'win32', stdio: 'ignore' }).unref()
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

function getProxyAgent (proxy) {
  if (proxy) {
    const agent = new ProxyAgent(proxy)
    return {
      http: agent,
      https: agent
    }
  }

  let agent

  const httpProxy = process.env.http_proxy || process.env.HTTP_PROXY
  if (httpProxy) {
    agent = agent || {}
    agent.http = new ProxyAgent(httpProxy)
  }

  const httpsProxy = process.env.https_proxy || process.env.HTTPS_PROXY
  if (httpsProxy) {
    agent = agent || {}
    agent.https = new ProxyAgent(httpsProxy)
  }

  return agent || false
}

function mkdirpSync (dir) {
  fs.mkdirSync(dir, { recursive: true })
}

class Updater {
  constructor (repo, prefix = 'app', customExe = false) {
    if (typeof repo !== 'string') throw new TypeError('Argument type error: new Updater(repo: string, prefix?: string, exe?: boolean)')
    if (typeof prefix !== 'string') throw new TypeError('Argument type error: new Updater(repo: string, prefix?: string, exe?: boolean)')
    if (typeof customExe !== 'boolean') throw new TypeError('Argument type error: new Updater(repo: string, prefix?: string, exe?: boolean)')
    this.repo = repo
    this.info = null
    this.reqObj = null
    this.prefix = prefix
    this.customExe = customExe
    this.proxy = ''
    this.headers = {
      'User-Agent': 'electron-github-asar-updater'
    }
    this.got = got.extend({
      headers: this.headers,
      responseType: 'json'
    })

    if (app.isPackaged) {
      if (!this.customExe) {
        if (!fs.existsSync(getPath('./updater/index.js')) || !fs.existsSync(getPath('./updater/package.json'))) {
          mkdirpSync(updater)
          fs.writeFileSync(getPath('./updater/index.js'), updaterScript)
          fs.writeFileSync(getPath('./updater/package.json'), JSON.stringify({ main: './index.js' }))
        }
      }
      
      if (fs.existsSync(dotPatch)) {
        this.relaunch()
      }
    }
  }

  setProxy (proxy) {
    this.proxy = proxy
  }

  relaunch () {
    if (app.isPackaged) {
      if (this.customExe) {
        app.exit()
      } else {
        if (fs.existsSync(updater)) fs.renameSync(updater, getPath('app'))
        app.relaunch()
        app.exit()
      }
    }
  }

  isReadyToDownload () {
    return !!this.info && !!this.info.appZipUrl
  }

  getUpdateInfo () {
    return this.info
  }

  abort () {
    if (this.reqObj) {
      this.reqObj.abort()
      this.reqObj = null
    }
  }

  download (onProgress) {
    if (!app.isPackaged) return Promise.resolve(false)

    if (!this.isReadyToDownload()) return Promise.reject(`No update or target file \`${this.prefix}-\${platform}-\${arch}.zip\` not found.`)
    this.abort()
    return new Promise((resolve, reject) => {
      const p = getPath('app.zip')
      let start = 0
      const d = tybysDownloader.Downloader.download(this.info.appZipUrl, {
        dir: path.dirname(p),
        out: path.basename(p),
        headers: this.headers,
        agent: this.proxy
      })
      const base = path.parse(d.path).base

      d.on('progress', (progress) => {
        if (typeof onProgress === 'function') {
          if (progress.completedLength === 0 || progress.percent === 100) {
            onProgress({
              name: base,
              current: progress.completedLength,
              max: progress.totalLength,
              loading: progress.percent
            })
          } else {
            const now = Date.now()
            if (now - start > 100) {
              start = now
              onProgress({
                name: base,
                current: progress.completedLength,
                max: progress.totalLength,
                loading: progress.percent
              })
            }
          }
        }
      })
      this.reqObj = d

      d.whenStopped().then(() => {
        resolve(d.path)
      }).catch(err => {
        if (err.code != null) {
          if (err.code === tybysDownloader.DownloadErrorCode.ABORT) {
            resolve('')
            return
          }
          if (err.code === tybysDownloader.DownloadErrorCode.FILE_EXISTS) {
            resolve(d.path)
            return
          }
        }
        reject(err)
      })
    }).then(zipPath => {
      if (!zipPath) return false
      if (!fs.existsSync(dotPatch)) mkdirpSync(dotPatch)
      return unzip(zipPath, dotPatch).then(() => {
        fs.unlinkSync(zipPath)
        return true
      })
    })
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

    const agent = getProxyAgent(this.proxy || '')
    return Promise.all([
      this.got.get(releases.url, { agent }),
      this.got.get(tags.url, { agent })
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

      // const appZip = latest.assets.filter((a) => a.name === `${this.prefix}-${process.platform}-${process.arch}.zip`)[0]
      const appZip = latest.assets.filter((a) => (new RegExp(`^${this.prefix}(-.*)?-${process.platform}-${process.arch}\\.zip$`)).test(a.name))[0]
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

module.exports = Updater
