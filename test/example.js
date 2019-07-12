const Updater = require('electron-github-asar-updater')

const { ipcMain } = require('electron')

module.exports = function () {
  const updater = new Updater('toyobayashi/electron-github-asar-updater', 'resources')

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
