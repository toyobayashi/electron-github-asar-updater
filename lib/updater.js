module.exports = `
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
    const ls = fs.readdirSync(dotPatch).map(file => path.join(dotPatch, file))
    for (let i = 0; i < ls.length; i++) {
      fs.renameSync(ls[i], getPath(path.basename(ls[i])))
    }
    removeSync(dotPatch)
  }
  fs.renameSync(getPath('app'), getPath('updater'))
  relaunch()
}

main()
`
