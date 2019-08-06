const Updater = require('electron-github-asar-updater')

const { ipcMain } = require('electron')
const path = require('path')

function getPath (...relative) {
  return path.join(process.resourcesPath, ...relative)
}

module.exports = function () {
  const updater = new Updater('toyobayashi/electron-github-asar-updater', 'resources', true)

  ipcMain.on('update', async function (ev) {
    try {
      await updater.check()
    } catch (err) {
      ev.sender.send('update-error', err.message)
      return
    }
    
    const info = updater.getUpdateInfo()
    if (info) {
      console.log(info)
      if (updater.isReadyToDownload()) {
        const downloadResult = await updater.download(({ name, current, max, loading }) => {
          ev.sender.send('update-downloading', { name, current, max, loading })
        })

        if (downloadResult) {
          require('child_process').spawn(
            // build /exe first, then place executable to resources/updater
            process.platform === 'win32' ? path.join(__dirname, '../updater/updater.exe') : path.join(__dirname, '../updater/updater'),
            process.platform === 'win32' ? [
              getPath('.patch'),
              getPath(),
              [process.argv0, ...process.argv.slice(1)].join(' ')
            ] : [
              getPath('.patch'),
              getPath(),
              process.argv0,
              ...process.argv.slice(1)
            ], { detached: process.platform === 'win32', stdio: 'ignore' }
          ).unref()
          updater.relaunch()
        } else {
          ev.sender.send('update-error', 'download aborted.')
        }
      }
    } else {
      ev.sender.send('update-message', 'No update.')
    }
  })
}
