# electron-github-asar-updater

This package works after Electron app is packaged.

electron-github-asar-updater v2.x require Electron version >= 2.0.0

## Usage

1. Default

    ``` js
    // Electron main process
    const Updater = require('electron-github-asar-updater')

    /**
     * Write file
     * ${process.resourcesPath}/updater/index.js
     * ${process.resourcesPath}/updater/package.json
     */
    const updater = new Updater('githubUser/repoName', '{{prefix}}');

    (async function () {
      /**
       * Check latest github release.
       * 
       * Tag name must be "vX.X.X".
       * For example: v2.3.3
       * 
       * Asar zip name must be `{{prefix}}-${process.platform}-${process.arch}.zip`
       * For example: {{prefix}}-win32-ia32.zip {{prefix}}-linux-x64.zip
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

      const info = updater.getUpdateInfo()
      if (info) { // newer version found
        
        console.log(info)
        if (updater.isReadyToDownload()) { // exists `resources-${process.platform}-${process.arch}.zip`
          /**
           * Download `resources-${process.platform}.zip` and unzip to
           * `${process.resourcesPath}/.patch`
           */
          const downloadResult = await updater.download(({ name, current, max, loading }) => {
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

          if (downloadResult) { // Success
            /**
             * 1. Rename `${process.resourcesPath}/updater` to `${process.resourcesPath}/app`
             * 2. Relaunch. Copy `${process.resourcesPath}/.patch/*` to `${process.resourcesPath}`
             * 3. Remove `${process.resourcesPath}/.patch`
             * 4. Rename `${process.resourcesPath}/app` to `${process.resourcesPath}/updater`
             * 5. Relaunch
             */ 
            updater.relaunch()
          } else {
            console.log('download aborted.')
          }
        }
      }
    })()
    ```

2. Use custom updater executable

    ``` js
    // Electron main process
    const Updater = require('electron-github-asar-updater')

    // use custom updater executable
    const updater = new Updater('githubUser/repoName', '{{prefix}}', true);
    (async function () {

      await updater.check({
        prerelease: -1
      })

      const info = updater.getUpdateInfo()
      if (info) {
        if (updater.isReadyToDownload()) {
          const downloadResult = await updater.download(({ name, current, max, loading }) => {})
          if (downloadResult) {
            // run executable and exit app, relaunch app by your program
            require('child_process').spawn(/* ... */).unref()
            updater.relaunch() // just call app.exit()
          } else {
            console.log('download aborted.')
          }
        }
      }
    })()
    ```
