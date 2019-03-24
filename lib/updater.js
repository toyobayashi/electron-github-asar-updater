module.exports = `
const path = require('path')
const fs = process.versions.electron ? require('original-fs') : require('fs')

let app = null
let isElectronEnvironment = true
try {
  app = require('electron').app || require('electron').remote.app
} catch (_) {
  isElectronEnvironment = false
  app = {
    relaunch () {
      require('child_process').spawn(process.argv0, process.argv.slice(1), { detached: process.platform === 'win32', stdio: 'ignore' }).unref()
    },
    exit (number) { process.exit(number) }
  }
}

function getPath (...relative) {
  return isElectronEnvironment ? path.join(process.resourcesPath, ...relative) : path.join(__dirname, '..', ...relative)
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

function mdSync (p) {
  const dir = path.dirname(p)
  if (!fs.existsSync(dir)) {
    mdSync(dir)
  } else {
    if (!fs.statSync(dir).isDirectory()) throw new Error(\`"$\{path.resolve(dir)}" is not a directory.\`)
  }
  fs.mkdirSync(p)
}

function copySync (src, dest) {
  const stat = fs.statSync(src)
  if (stat.isDirectory()) {
    if (!fs.existsSync(dest)) mdSync(dest)
    const ls = fs.readdirSync(src).map(file => path.join(src, file))
    for (let i = 0; i < ls.length; i++) {
      copySync(ls[i], path.join(dest, path.basename(ls[i])))
    }
  } else {
    if (!fs.existsSync(path.dirname(dest))) mdSync(path.dirname(dest))
    fs.copyFileSync(src, dest)
  }
}

function main () {
  const dotPatch = getPath('.patch')
  if (fs.existsSync(dotPatch)) {
    const ls = fs.readdirSync(dotPatch).map(file => path.join(dotPatch, file))
    for (let i = 0; i < ls.length; i++) {
      copySync(ls[i], getPath(path.basename(ls[i])))
    }
    removeSync(dotPatch)
  }
  fs.renameSync(getPath('app'), getPath('updater'))
  relaunch()
}

main()
`
