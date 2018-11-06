declare interface Info {
  version: string
  commit: string
  zipUrl: string | null
  exeUrl: string | null
  appZipUrl: string | null
  release: {
    [key: string]: any
    url: string
    assets_url: string
    upload_url: string
    html_url: string
    id: number
    node_id: string
    tag_name: string
    target_commitish: string
    name: string
    draft: boolean
    author: {
      [key: string]: any
      login: string
      id: number
      node_id: string
      avatar_url: string
      gravatar_id: string
      url: string
      html_url: string
      followers_url: string
      following_url: string
      gists_url: string
      starred_url: string
      subscriptions_url: string
      organizations_url: string
      repos_url: string
      events_url: string
      received_events_url: string
      type: string
      site_admin: boolean
    }
    prerelease: boolean
    created_at: string
    published_at: string
    assets: {
      [key: string]: any
      url: string
      id: number
      node_id: string
      name: string
      label: any
      uploader: {
        [key: string]: any
        login: string
        id: number
        node_id: string
        avatar_url: string
        gravatar_id: string
        url: string
        html_url: string
        followers_url: string
        following_url: string
        gists_url: string
        starred_url: string
        subscriptions_url: string
        organizations_url: string
        repos_url: string
        events_url: string
        received_events_url: string
        type: string
        site_admin: boolean
      }
      content_type: string
      state: string
      size: number
      download_count: number
      created_at: string
      updated_at: string
      browser_download_url: string
    }[]
    tarball_url: string
    zipball_url: string
    body: string
  }
  tag: {
    [key: string]: any
    name: string
    zipball_url: string
    tarball_url: string
    commit: {
      [key: string]: any
      sha: string
      url: string
    }
    node_id: string
  }
}

declare class Updater {
  constructor (repo: string)
  relaunch (): void
  hasUpdate (): boolean
  getUpdateInfo (): Info | null
  download (onProgress?: (status: { name: string; current: number; max: number; loading: number }) => void): Promise<number>
  check (options?: { prerelease?: -1 | 0 | 1 }): Promise<Info | null>
}

declare namespace Updater {}

export = Updater
