(function () {
  const ipcRenderer = require('electron').ipcRenderer

  const updatebtn = document.getElementById('update')
  const messagep = document.getElementById('message')

  ipcRenderer.on('update-error', function (ev, message) {
    messagep.innerHTML = `Error: ${message}`
  })

  ipcRenderer.on('update-message', function (ev, message) {
    messagep.innerHTML = `Message: ${message}`
  })

  ipcRenderer.on('update-downloading', function (ev, progress) {
    messagep.innerHTML = `Downloading: ${Math.floor(progress.loading * 100) / 100}%`
  })
  
  updatebtn.addEventListener('click', function () {
    require('electron').ipcRenderer.send('update')
  }, false)
})()
