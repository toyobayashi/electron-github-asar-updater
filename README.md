# electron-github-asar-updater

This package works after Electron app is packaged.

electron-github-asar-updater v2.x require Electron version >= 2.0.0

## Usage

``` js
// Electron main process
const Updater = require('electron-github-asar-updater')

/**
 * Write file
 * ${process.resourcesPath}/updater/index.js
 * ${process.resourcesPath}/updater/package.json
 */
const updater = new Updater('githubUser/repoName');

(async function () {
  /**
   * Check latest github release.
   * 
   * Tag name must be "vX.X.X".
   * For example: v2.3.3
   * 
   * Asar zip name must be `app-${process.platform}-${process.arch}.zip`
   * For example: app-win32-ia32.zip app-linux-x64.zip
   * 
   * Full asset name example:
   * myapp-v2.3.3-win32-x64.zip
   * myapp-v2.3.3-win32-ia32.exe
   * myapp-v2.3.3-linux-x64.deb
   */
  await updater.check({
    /**
     * -1: ignore pre-release (default)
     * 0:  include pre-release
     * 1:  check prerelease only
     */
    prerelease: -1
  })

  if (updater.hasUpdate()) { // or if (updater.getUpdateInfo())
    console.log(updater.getUpdateInfo())

    /**
     * Download `app-${process.platform}.zip` and unzip to
     * `${process.resourcesPath}/.patch`
     */
    await updater.download(({ name, current, max, loading }) => {
      /**
       * {
       *   name: string;
       *   current: number;
       *   max: number;
       *   loading: number;
       * }
       * 
       * do something, return void
       */
    })

    /**
     * 1. Rename `${process.resourcesPath}/updater` to `${process.resourcesPath}/app`
     * 2. Relaunch. Copy `${process.resourcesPath}/.patch/*` to `${process.resourcesPath}`
     * 3. Remove `${process.resourcesPath}/.patch`
     * 4. Rename `${process.resourcesPath}/app` to `${process.resourcesPath}/updater`
     * 5. Relaunch
     */ 
    updater.relaunch()
  }
})()
```
