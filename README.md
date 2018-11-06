# electron-github-asar-updater

This package works after Electron app is packaged.

## Usage

``` js
// Electron main process
const Updater = require('electron-github-asar-updater')

/**
 * Write file
 * ${process.resourcesPath}/updater/index.js
 * ${process.resourcesPath}/updater/package.json
 */
const updater = new Updater('githubUser/repoName')

(async function () {
  /**
   * Check update.
   * Tag name must be "vX.X.X"
   * Asar zip name must be `app-${process.platform}.zip`
   * Full asset name must be
   * `${app.getName()}-v${app.getVersion()}-${process.platform}-${process.arch}` +
   * .zip or .exe or .deb
   */
  await updater.check()

  if (updater.hasUpdate()) { // or if (updater.getUpdateInfo())
    console.log(updater.getUpdateInfo())

    /**
     * Download `app-${process.platform}.zip` and unzip to
     * `${process.resourcesPath}/.patch`
     */
    await updater.download(({ name, current, max, loading }) => {
      // do something, return void
    })

    /**
     * 1. Rename `${process.resourcesPath}/updater` to `${process.resourcesPath}/app`
     * 2. Relaunch. Move `${process.resourcesPath}/.patch/*` to `${process.resourcesPath}`
     * 3. Remove `${process.resourcesPath}/.patch`
     * 4. Rename `${process.resourcesPath}/app` to `${process.resourcesPath}/updater`
     * 5. Relaunch
     */ 
    updater.relaunch()
  }
})()
```
