declare interface Info {
  version: string
  commit: string
  zipUrl: string | null
  exeUrl: string | null
  appZipUrl: string | null
}

declare class Updater {
  constructor (repo: string)
  relaunch (): void
  hasUpdate (): boolean
  getUpdateInfo (): Info | null
  download (onProgress?: (status: { name: string; current: number; max: number; loading: number }) => void): Promise<number>
  check (): Promise<Info | null>
}

declare namespace Updater {}

export = Updater
